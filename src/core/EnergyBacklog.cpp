#include "core/EnergyBacklog.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <stddef.h>
#include <cmath>
#include <initializer_list>
#include <string.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>

#include "core/SolarState.h"
#include "descriptors.h"

extern void writeLog(const char *format, ...);

namespace
{
constexpr const char *BACKLOG_PARTITION = "backlog";
constexpr const char *BACKLOG_FILE = "/energy.bin";
constexpr const char *BACKLOG_TEMP_FILE = "/energy.tmp";
constexpr uint32_t RECORD_MAGIC = 0x454E5247UL; // "ENRG"
constexpr uint16_t RECORD_VERSION = 1;
constexpr unsigned long CAPTURE_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr size_t MAX_BACKLOG_BYTES = 192UL * 1024UL;
constexpr size_t KEEP_BACKLOG_BYTES = 96UL * 1024UL;

constexpr const char *BATTERY_SLOT_A = "/battery_a.bin";
constexpr const char *BATTERY_SLOT_B = "/battery_b.bin";
constexpr uint32_t BATTERY_RECORD_MAGIC = 0x42415445UL; // "BATE"
constexpr uint16_t BATTERY_RECORD_VERSION = 1;
constexpr unsigned long BATTERY_PUBLISH_INTERVAL_MS = 1000UL;
constexpr unsigned long BATTERY_CHECKPOINT_RETRY_MS = 10000UL;
constexpr unsigned long BATTERY_CHECKPOINT_INTERVAL_MS = 2UL * 60UL * 1000UL;

const char *const ENERGY_KEYS[] = {
    DESCR_PV_Generation_Sum,
    DESCR_PV_Generation_Year,
    DESCR_PV_Generation_Month,
    DESCR_PV_Generation_Day,
    DESCR_AC_In_Generation_Sum,
    DESCR_AC_In_Generation_Year,
    DESCR_AC_In_Generation_Month,
    DESCR_AC_In_Generation_Day,
};

constexpr size_t ENERGY_KEY_COUNT = sizeof(ENERGY_KEYS) / sizeof(ENERGY_KEYS[0]);

struct EnergyRecord
{
    // Order fields so the 64-bit counters stay naturally aligned on every
    // ESP32 family while the on-flash record remains exactly 80 bytes.
    uint32_t magic;
    uint32_t sequence;
    int64_t values[ENERGY_KEY_COUNT];
    uint16_t version;
    uint16_t mask;
    uint32_t checksum;
};

static_assert(ENERGY_KEY_COUNT == 8, "Energy backlog format expects 8 counters");
static_assert(sizeof(EnergyRecord) == 80, "Unexpected EnergyRecord size");

// V1 files are imported without deleting or changing them. V2 adds an atomic
// migration receipt and a pending flag, so a replay cannot add an offset twice.
struct LegacyBatteryEnergyRecord
{
    uint32_t magic;
    uint32_t sequence;
    uint64_t chargeMilliWh;
    uint64_t dischargeMilliWh;
    uint16_t version;
    uint16_t reserved;
    uint32_t checksum;
};
struct BatteryEnergyRecord
{
    uint32_t magic;
    uint32_t sequence;
    uint64_t chargeMilliWh;
    uint64_t dischargeMilliWh;
    uint8_t migrationId[16];
    uint16_t version;
    uint16_t flags;
    uint32_t checksum;
};
static_assert(sizeof(LegacyBatteryEnergyRecord) == 32, "Unexpected legacy battery record size");
static_assert(sizeof(BatteryEnergyRecord) == 48, "Unexpected battery record size");
constexpr uint16_t BATTERY_RECORD_VERSION_V2 = 2;
constexpr uint16_t BATTERY_PENDING_MIGRATION = 1;
constexpr double BATTERY_MAX_COUNTER_WH = 1.0e12;
const char *const BATTERY_FILES_V2[] = {"/battery_v2_a.bin", "/battery_v2_b.bin"};
const char *const BATTERY_NVS_KEYS[] = {"counter_a", "counter_b"};

uint32_t recordChecksum(const EnergyRecord &record)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&record);
    const size_t length = offsetof(EnergyRecord, checksum);

    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < length; ++i)
    {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }
    return hash;
}

bool recordValid(const EnergyRecord &record)
{
    return record.magic == RECORD_MAGIC &&
           record.version == RECORD_VERSION &&
           record.mask != 0 &&
           record.checksum == recordChecksum(record);
}


uint32_t batteryChecksum(const void *record, size_t length)
{
    const uint8_t *bytes = static_cast<const uint8_t *>(record);
    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < length; ++i)
    {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }
    return hash;
}

bool batteryRecordValid(const BatteryEnergyRecord &record)
{
    return record.magic == BATTERY_RECORD_MAGIC && record.version == BATTERY_RECORD_VERSION_V2 &&
           (record.flags & ~BATTERY_PENDING_MIGRATION) == 0 &&
           record.chargeMilliWh <= 1000000000000000ULL &&
           record.dischargeMilliWh <= 1000000000000000ULL &&
           record.checksum == batteryChecksum(&record, offsetof(BatteryEnergyRecord, checksum));
}

bool importLegacyBatteryRecord(const LegacyBatteryEnergyRecord &legacy, BatteryEnergyRecord &record)
{
    if (legacy.magic != BATTERY_RECORD_MAGIC || legacy.version != BATTERY_RECORD_VERSION ||
        legacy.checksum != batteryChecksum(&legacy, offsetof(LegacyBatteryEnergyRecord, checksum)) ||
        legacy.chargeMilliWh > 1000000000000000ULL || legacy.dischargeMilliWh > 1000000000000000ULL)
        return false;
    record = {};
    record.magic = legacy.magic;
    record.sequence = legacy.sequence;
    record.chargeMilliWh = legacy.chargeMilliWh;
    record.dischargeMilliWh = legacy.dischargeMilliWh;
    record.version = BATTERY_RECORD_VERSION_V2;
    record.checksum = batteryChecksum(&record, offsetof(BatteryEnergyRecord, checksum));
    return true;
}

bool newerBatterySequence(uint32_t a, uint32_t b)
{
    return static_cast<int32_t>(a - b) > 0;
}

bool readBatteryFile(const char *path, BatteryEnergyRecord &record, bool legacy = false)
{
    File file = LittleFS.open(path, "r");
    if (!file)
        return false;
    bool ok = false;
    if (legacy && file.size() == sizeof(LegacyBatteryEnergyRecord))
    {
        LegacyBatteryEnergyRecord old{};
        ok = file.read(reinterpret_cast<uint8_t *>(&old), sizeof(old)) == sizeof(old) &&
             importLegacyBatteryRecord(old, record);
    }
    else if (!legacy && file.size() == sizeof(record))
        ok = file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) == sizeof(record) && batteryRecordValid(record);
    file.close();
    return ok;
}

bool writeBatteryFile(const char *path, const BatteryEnergyRecord &record)
{
    File file = LittleFS.open(path, "w");
    if (!file)
        return false;
    const size_t written = file.write(reinterpret_cast<const uint8_t *>(&record), sizeof(record));
    file.flush();
    file.close();
    BatteryEnergyRecord check{};
    return written == sizeof(record) && readBatteryFile(path, check) && memcmp(&record, &check, sizeof(record)) == 0;
}

bool readBatteryNvs(Preferences &prefs, const char *key, BatteryEnergyRecord &record)
{
    return prefs.getBytesLength(key) == sizeof(record) &&
           prefs.getBytes(key, &record, sizeof(record)) == sizeof(record) && batteryRecordValid(record);
}

bool writeBatteryNvs(const char *key, const BatteryEnergyRecord &record)
{
    Preferences prefs;
    if (!prefs.begin("batenergy", false))
        return false;
    const size_t written = prefs.putBytes(key, &record, sizeof(record));
    BatteryEnergyRecord check{};
    const bool ok = written == sizeof(record) && readBatteryNvs(prefs, key, check) && memcmp(&record, &check, sizeof(record)) == 0;
    prefs.end();
    return ok;
}

bool erasedBacklogPartition()
{
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, BACKLOG_PARTITION);
    if (partition == nullptr)
        return false;
    uint8_t bytes[256];
    for (size_t offset = 0; offset < partition->size; offset += sizeof(bytes))
    {
        const size_t length = (partition->size - offset) < sizeof(bytes) ? partition->size - offset : sizeof(bytes);
        if (esp_partition_read(partition, offset, bytes, length) != ESP_OK)
            return false;
        for (size_t i = 0; i < length; ++i)
            if (bytes[i] != 0xff)
                return false;
    }
    return true;
}

bool parseBatteryMigrationId(const char *text, uint8_t id[16])
{
    if (text == nullptr || strlen(text) != 32)
        return false;
    bool nonzero = false;
    for (size_t i = 0; i < 16; ++i)
    {
        unsigned value = 0;
        for (size_t j = 0; j < 2; ++j)
        {
            const char c = text[i * 2 + j];
            const unsigned digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : 16;
            if (digit > 15)
                return false;
            value = value * 16 + digit;
        }
        id[i] = static_cast<uint8_t>(value);
        nonzero = nonzero || value != 0;
    }
    return nonzero;
}

void appendEnergyPartition(JsonObject target, const esp_partition_t *partition)
{
    target["present"] = partition != nullptr;
    if (partition != nullptr)
    {
        target["label"] = partition->label;
        target["address"] = partition->address;
        target["size"] = partition->size;
        target["type"] = static_cast<unsigned>(partition->type);
        target["subtype"] = static_cast<unsigned>(partition->subtype);
    }
}

bool readEnergyValues(SolarState &state, EnergyRecord &record)
{
    JsonDocument snapshot;
    if (!state.snapshotTo(snapshot))
        return false;
    JsonObjectConst live = snapshot["LiveData"].as<JsonObjectConst>();

    record.mask = 0;
    for (size_t i = 0; i < ENERGY_KEY_COUNT; ++i)
    {
        JsonVariantConst value = live[ENERGY_KEYS[i]];
        if (value.isNull())
        {
            continue;
        }

        record.values[i] = value.as<int64_t>();
        record.mask |= static_cast<uint16_t>(1U << i);
    }

    return record.mask != 0;
}

bool publishEnergyRecord(PubSubClient &client, const String &baseTopic, const EnergyRecord &record)
{
    for (size_t i = 0; i < ENERGY_KEY_COUNT; ++i)
    {
        const uint16_t bit = static_cast<uint16_t>(1U << i);
        if ((record.mask & bit) == 0)
        {
            continue;
        }

        const String topic = baseTopic + "/LiveData/" + ENERGY_KEYS[i];
        char payload[32];
        snprintf(payload, sizeof(payload), "%lld", static_cast<long long>(record.values[i]));

        if (!client.publish(topic.c_str(), payload, true))
        {
            return false;
        }

        client.loop();
    }

    return true;
}
} // namespace

EnergyBacklog::EnergyBacklog()
    : _ready(false),
      _offlineActive(false),
      _haveLastValues(false),
      _replayActive(false),
      _lastCaptureMs(0),
      _nextSequence(1),
      _replayOffset(0),
      _lastMask(0),
      _batteryChargeWh(0.0),
      _batteryDischargeWh(0.0),
      _batteryLastIntegrateMs(0),
      _batteryLastCheckpointMs(0),
      _batteryCheckpointSequence(0),
      _batteryDirection(0),
      _batteryDirty(false)
{
    memset(_lastValues, 0, sizeof(_lastValues));
}

void EnergyBacklog::begin()
{
    inspectBatteryPartitions();
    _batteryLastError = "";
    _batteryStorageWarning = "";
    const bool partitionPresent = _batteryPartitions["backlog"]["present"] | false;
    _ready = partitionPresent && LittleFS.begin(false, "/littlefs", 5, BACKLOG_PARTITION);
    if (!_ready && partitionPresent && erasedBacklogPartition())
    {
        // Only an entirely erased, unused partition may be initialized here.
        // A damaged filesystem is never formatted as an implicit recovery step.
        _ready = LittleFS.format() && LittleFS.begin(false, "/littlefs", 5, BACKLOG_PARTITION);
        _batteryPartitions["backlog"]["initializedErasedPartition"] = _ready;
    }
    if (!_ready)
        _batteryStorageWarning = partitionPresent ? "backlog_mount_failed" : "backlog_partition_missing";

    Preferences prefs;
    _batteryNvsAvailable = prefs.begin("batenergy", false);
    if (_batteryNvsAvailable)
    {
        _batteryPreferNvs = prefs.getBool("prefer_nvs", false);
        _batteryLegacyInitialized = prefs.getBool("initialized", false);
        _batteryFactoryResetPending = prefs.getBool("factory_reset", false);
        if (!_ready && !_batteryPreferNvs)
        {
            // An inaccessible filesystem may contain a higher old sequence.
            // Once recovery starts in NVS, do not later resurrect those files
            // simply because the filesystem becomes readable again.
            _batteryPreferNvs = prefs.putBool("prefer_nvs", true) == 1;
            if (!_batteryPreferNvs)
            {
                _batteryNvsAvailable = false;
                _batteryStorageWarning = "nvs_backend_marker_write_failed";
            }
        }
        prefs.end();
    }
    _batteryStore = _batteryPreferNvs ? (_batteryNvsAvailable ? BatteryStore::Nvs : BatteryStore::None)
                                    : (_ready ? BatteryStore::LittleFs : (_batteryNvsAvailable ? BatteryStore::Nvs : BatteryStore::None));
    resetBatteryEnergyIfFactoryReset();
    if (!_batteryFactoryResetPending)
        loadBatteryEnergyCheckpoint();
    _batteryHaveIntegrationTime = false;
    _batteryHaveSample = false;
    _batteryActivePowerW = 0;
    _batteryHaveStatePublish = false;
    _batteryLastCheckpointMs = millis();
    if (_batteryDirty && !_batteryRecoveryRequired)
        saveBatteryEnergyCheckpoint(true);
    writeLog("[BatteryEnergy] storage=%s pending=%d restored=%d error=%s", _batteryStore == BatteryStore::LittleFs ? "littlefs" : _batteryStore == BatteryStore::Nvs ? "nvs" : "none", _batteryMigrationPending, _batteryLoadedCheckpoint, _batteryLastError.c_str());
    if (!_ready)
    {
        writeLog("[EnergyBacklog] LittleFS unavailable; inverter-counter backlog disabled");
        return;
    }
    repairTrailingPartialRecord();
    File file = LittleFS.open(BACKLOG_FILE, "r");
    if (file)
    {
        EnergyRecord record{};
        while (file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) == sizeof(record))
            if (recordValid(record) && record.sequence >= _nextSequence)
                _nextSequence = record.sequence + 1;
        file.close();
    }
}

void EnergyBacklog::captureIfNeeded(SolarState &state, bool mqttOffline, unsigned long nowMs)
{
    if (!_ready)
    {
        return;
    }

    if (!mqttOffline)
    {
        _offlineActive = false;
        return;
    }

    appendCurrentSnapshot(state, nowMs, !_offlineActive);
    _offlineActive = true;
}

bool EnergyBacklog::appendCurrentSnapshot(SolarState &state, unsigned long nowMs, bool force)
{
    EnergyRecord record{};
    record.magic = RECORD_MAGIC;
    record.version = RECORD_VERSION;
    record.sequence = _nextSequence;

    if (!readEnergyValues(state, record))
    {
        return false;
    }

    bool changed = !_haveLastValues || record.mask != _lastMask;
    bool decreased = false;

    for (size_t i = 0; i < ENERGY_KEY_COUNT; ++i)
    {
        const uint16_t bit = static_cast<uint16_t>(1U << i);
        if ((record.mask & bit) == 0)
        {
            continue;
        }

        if (!_haveLastValues || (_lastMask & bit) == 0 || record.values[i] != _lastValues[i])
        {
            changed = true;
        }

        if (_haveLastValues && (_lastMask & bit) != 0 && record.values[i] < _lastValues[i])
        {
            decreased = true;
        }
    }

    if (!changed)
    {
        return false;
    }

    if (!force && !decreased && (nowMs - _lastCaptureMs) < CAPTURE_INTERVAL_MS)
    {
        return false;
    }

    record.checksum = recordChecksum(record);

    if (!compactIfNeeded(sizeof(record)))
    {
        return false;
    }

    File file = LittleFS.open(BACKLOG_FILE, "a");
    if (!file)
    {
        writeLog("[EnergyBacklog] Failed to open backlog for append");
        return false;
    }

    const size_t written = file.write(reinterpret_cast<const uint8_t *>(&record), sizeof(record));
    file.flush();
    file.close();

    if (written != sizeof(record))
    {
        writeLog("[EnergyBacklog] Incomplete backlog write: %u/%u",
                 static_cast<unsigned>(written),
                 static_cast<unsigned>(sizeof(record)));
        repairTrailingPartialRecord();
        return false;
    }

    ++_nextSequence;
    _lastCaptureMs = nowMs;
    _lastMask = record.mask;
    memcpy(_lastValues, record.values, sizeof(_lastValues));
    _haveLastValues = true;
    return true;
}

bool EnergyBacklog::compactIfNeeded(size_t incomingBytes)
{
    File source = LittleFS.open(BACKLOG_FILE, "r");
    if (!source)
    {
        return true;
    }

    const size_t sourceSize = source.size();
    if ((sourceSize + incomingBytes) <= MAX_BACKLOG_BYTES)
    {
        source.close();
        return true;
    }

    const size_t recordSize = sizeof(EnergyRecord);
    size_t keepBytes = KEEP_BACKLOG_BYTES - (KEEP_BACKLOG_BYTES % recordSize);
    if (keepBytes > sourceSize)
    {
        keepBytes = sourceSize - (sourceSize % recordSize);
    }

    const size_t startOffset = sourceSize - keepBytes;
    if (!source.seek(startOffset, SeekSet))
    {
        source.close();
        writeLog("[EnergyBacklog] Compaction seek failed");
        return false;
    }

    LittleFS.remove(BACKLOG_TEMP_FILE);
    File target = LittleFS.open(BACKLOG_TEMP_FILE, "w");
    if (!target)
    {
        source.close();
        writeLog("[EnergyBacklog] Compaction temp open failed");
        return false;
    }

    uint8_t buffer[512];
    size_t remaining = keepBytes;
    bool ok = true;

    while (remaining > 0)
    {
        const size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const size_t read = source.read(buffer, chunk);
        if (read != chunk || target.write(buffer, read) != read)
        {
            ok = false;
            break;
        }
        remaining -= read;
    }

    target.flush();
    target.close();
    source.close();

    if (!ok)
    {
        LittleFS.remove(BACKLOG_TEMP_FILE);
        writeLog("[EnergyBacklog] Compaction copy failed");
        return false;
    }

    LittleFS.remove(BACKLOG_FILE);
    if (!LittleFS.rename(BACKLOG_TEMP_FILE, BACKLOG_FILE))
    {
        writeLog("[EnergyBacklog] Compaction rename failed");
        return false;
    }

    writeLog("[EnergyBacklog] Backlog compacted to %u bytes", static_cast<unsigned>(keepBytes));
    return true;
}

bool EnergyBacklog::repairTrailingPartialRecord()
{
    File source = LittleFS.open(BACKLOG_FILE, "r");
    if (!source)
    {
        return true;
    }

    const size_t recordSize = sizeof(EnergyRecord);
    const size_t currentSize = source.size();
    const size_t alignedSize = currentSize - (currentSize % recordSize);

    if (alignedSize == currentSize)
    {
        source.close();
        return true;
    }

    // This Arduino-ESP32 FS implementation does not expose File::truncate().
    // Recover from a torn final write by rebuilding only the newest aligned
    // records into the temporary file. Keeping at most KEEP_BACKLOG_BYTES also
    // guarantees enough free space for the recovery copy.
    size_t keepBytes = KEEP_BACKLOG_BYTES - (KEEP_BACKLOG_BYTES % recordSize);
    if (keepBytes > alignedSize)
    {
        keepBytes = alignedSize;
    }

    const size_t startOffset = alignedSize - keepBytes;
    if (!source.seek(startOffset, SeekSet))
    {
        source.close();
        writeLog("[EnergyBacklog] Partial-tail recovery seek failed");
        return false;
    }

    LittleFS.remove(BACKLOG_TEMP_FILE);
    File target = LittleFS.open(BACKLOG_TEMP_FILE, "w");
    if (!target)
    {
        source.close();
        writeLog("[EnergyBacklog] Partial-tail recovery temp open failed");
        return false;
    }

    uint8_t buffer[512];
    size_t remaining = keepBytes;
    bool ok = true;

    while (remaining > 0)
    {
        const size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const size_t read = source.read(buffer, chunk);
        if (read != chunk || target.write(buffer, read) != read)
        {
            ok = false;
            break;
        }
        remaining -= read;
    }

    target.flush();
    target.close();
    source.close();

    if (!ok)
    {
        LittleFS.remove(BACKLOG_TEMP_FILE);
        writeLog("[EnergyBacklog] Partial-tail recovery copy failed");
        return false;
    }

    LittleFS.remove(BACKLOG_FILE);
    if (!LittleFS.rename(BACKLOG_TEMP_FILE, BACKLOG_FILE))
    {
        writeLog("[EnergyBacklog] Partial-tail recovery rename failed");
        return false;
    }

    writeLog("[EnergyBacklog] Recovered torn write; dropped %u trailing bytes",
             static_cast<unsigned>(currentSize - alignedSize));
    return true;
}

void EnergyBacklog::resetBatteryEnergyIfFactoryReset()
{
    if (!_batteryFactoryResetPending)
        return;
    _batteryMigrationPending = true;
    Preferences prefs;
    if (!_batteryNvsAvailable || !prefs.begin("batenergy", false))
    {
        _batteryStore = BatteryStore::None;
        _batteryLastError = "factory_reset_nvs_unavailable";
        return;
    }
    // A deliberate factory reset is explicit. Missing legacy initialization
    // metadata alone must never delete an otherwise valid checkpoint.
    if (prefs.putBool("prefer_nvs", true) != 1)
    {
        prefs.end();
        _batteryStore = BatteryStore::None;
        _batteryLastError = "factory_reset_marker_write_failed";
        return;
    }
    _batteryPreferNvs = true;
    for (const char *key : BATTERY_NVS_KEYS)
    {
        if (prefs.isKey(key) && !prefs.remove(key))
        {
            prefs.end();
            _batteryStore = BatteryStore::None;
            _batteryMigrationPending = true;
            _batteryLastError = "factory_reset_checkpoint_remove_failed";
            return;
        }
    }
    prefs.end();
    if (_ready)
    {
        LittleFS.remove(BATTERY_SLOT_A);
        LittleFS.remove(BATTERY_SLOT_B);
        LittleFS.remove(BATTERY_FILES_V2[0]);
        LittleFS.remove(BATTERY_FILES_V2[1]);
    }
    _batteryStore = BatteryStore::Nvs;
    _batteryChargeWh = _batteryDischargeWh = 0;
    _batteryCheckpointSequence = _batteryWriteSequence = 0;
    _batteryNvsSlot = _batteryFileSlot = -1;
    memset(_batteryMigrationId, 0, sizeof(_batteryMigrationId));
    _batteryMigrationPending = false;
    _batteryDirty = true;
}

void EnergyBacklog::loadBatteryEnergyCheckpoint()
{
    BatteryEnergyRecord best{};
    bool found = false, legacySelected = false;
    uint32_t fileSequence = 0, nvsSequence = 0;
    _batteryFileSlot = _batteryNvsSlot = -1;
    _batteryHadRecords = false;
    auto consider = [&](const BatteryEnergyRecord &record, bool legacy)
    {
        if (!found || (!legacy && legacySelected) || (legacy == legacySelected && newerBatterySequence(record.sequence, best.sequence)))
        {
            best = record;
            found = true;
            legacySelected = legacy;
        }
    };
    if (_ready && !_batteryPreferNvs)
    {
        for (int i = 0; i < 2; ++i)
        {
            BatteryEnergyRecord record{};
            _batteryHadRecords = _batteryHadRecords || LittleFS.exists(BATTERY_FILES_V2[i]);
            if (readBatteryFile(BATTERY_FILES_V2[i], record))
            {
                consider(record, false);
                if (_batteryFileSlot < 0 || newerBatterySequence(record.sequence, fileSequence))
                {
                    _batteryFileSlot = i;
                    fileSequence = record.sequence;
                }
            }
        }
        for (const char *name : {BATTERY_SLOT_A, BATTERY_SLOT_B})
        {
            BatteryEnergyRecord record{};
            _batteryHadRecords = _batteryHadRecords || LittleFS.exists(name);
            if (readBatteryFile(name, record, true))
                consider(record, true);
        }
    }
    Preferences prefs;
    if (_batteryNvsAvailable && prefs.begin("batenergy", false))
    {
        for (int i = 0; i < 2; ++i)
        {
            BatteryEnergyRecord record{};
            _batteryHadRecords = _batteryHadRecords || prefs.isKey(BATTERY_NVS_KEYS[i]);
            if (readBatteryNvs(prefs, BATTERY_NVS_KEYS[i], record))
            {
                consider(record, false);
                if (_batteryNvsSlot < 0 || newerBatterySequence(record.sequence, nvsSequence))
                {
                    _batteryNvsSlot = i;
                    nvsSequence = record.sequence;
                }
            }
        }
        prefs.end();
    }
    _batteryLoadedCheckpoint = found;
    _batteryRecoveryRequired = !found && _batteryHadRecords;
    if (found)
    {
        _batteryChargeWh = static_cast<double>(best.chargeMilliWh) / 1000.0;
        _batteryDischargeWh = static_cast<double>(best.dischargeMilliWh) / 1000.0;
        _batteryCheckpointSequence = _batteryWriteSequence = best.sequence;
        memcpy(_batteryMigrationId, best.migrationId, sizeof(_batteryMigrationId));
        _batteryMigrationPending = (best.flags & BATTERY_PENDING_MIGRATION) != 0;
        _batteryLastSaveOk = true;
        // Import legacy records, or move the latest fallback record back to a
        // working preferred backend, using a verified new-format checkpoint.
        _batteryDirty = legacySelected || (_batteryStore == BatteryStore::LittleFs &&
                         (_batteryFileSlot < 0 || newerBatterySequence(best.sequence, fileSequence)));
    }
    else
    {
        _batteryChargeWh = _batteryDischargeWh = 0;
        _batteryCheckpointSequence = _batteryWriteSequence = 0;
        memset(_batteryMigrationId, 0, sizeof(_batteryMigrationId));
        _batteryMigrationPending = !_ready || _batteryPreferNvs || _batteryLegacyInitialized || _batteryHadRecords;
        _batteryDirty = true;
        if (_batteryRecoveryRequired)
            _batteryLastError = "no_valid_checkpoint_recovery_required";
    }
}

bool EnergyBacklog::saveBatteryEnergyCheckpoint(bool force)
{
    _batteryLastCheckpointAttemptMs = millis();
    _batteryHaveCheckpointAttempt = true;
    if (_batteryRecoveryRequired || _batteryMigrationUncertain)
    {
        _batteryLastSaveOk = false;
        return false;
    }
    if (_batteryStore == BatteryStore::None)
    {
        _batteryLastSaveOk = false;
        _batteryLastError = "battery_storage_unavailable";
        return false;
    }
    if (!_batteryDirty && !force)
        return true;
    if (!std::isfinite(_batteryChargeWh) || !std::isfinite(_batteryDischargeWh) ||
        _batteryChargeWh < 0 || _batteryDischargeWh < 0 ||
        _batteryChargeWh > BATTERY_MAX_COUNTER_WH || _batteryDischargeWh > BATTERY_MAX_COUNTER_WH)
    {
        _batteryLastSaveOk = false;
        _batteryLastError = "counter_out_of_range";
        return false;
    }
    BatteryEnergyRecord record{};
    record.magic = BATTERY_RECORD_MAGIC;
    record.sequence = ++_batteryWriteSequence;
    // A verified deliberate-restart checkpoint must not round a published
    // total down and look like a counter reset to Home Assistant. The upward
    // disk quantization is less than one milliWh and only carried in on boot.
    record.chargeMilliWh = static_cast<uint64_t>(std::ceil(_batteryChargeWh * 1000.0));
    record.dischargeMilliWh = static_cast<uint64_t>(std::ceil(_batteryDischargeWh * 1000.0));
    record.version = BATTERY_RECORD_VERSION_V2;
    record.flags = _batteryMigrationPending ? BATTERY_PENDING_MIGRATION : 0;
    memcpy(record.migrationId, _batteryMigrationId, sizeof(record.migrationId));
    record.checksum = batteryChecksum(&record, offsetof(BatteryEnergyRecord, checksum));
    bool ok = false;
    if (_batteryStore == BatteryStore::LittleFs)
    {
        const int slot = _batteryFileSlot == 0 ? 1 : 0;
        ok = writeBatteryFile(BATTERY_FILES_V2[slot], record);
        if (ok)
            _batteryFileSlot = slot;
        else if (_batteryNvsAvailable)
        {
            const int nvsSlot = _batteryNvsSlot == 0 ? 1 : 0;
            ok = writeBatteryNvs(BATTERY_NVS_KEYS[nvsSlot], record);
            if (ok)
            {
                _batteryNvsSlot = nvsSlot;
                _batteryStore = BatteryStore::Nvs;
                _batteryStorageWarning = "littlefs_checkpoint_failed_using_nvs";
            }
        }
    }
    else
    {
        const int slot = _batteryNvsSlot == 0 ? 1 : 0;
        ok = writeBatteryNvs(BATTERY_NVS_KEYS[slot], record);
        if (ok)
            _batteryNvsSlot = slot;
    }
    _batteryLastSaveOk = ok;
    if (!ok)
    {
        _batteryLastError = "checkpoint_write_or_verify_failed";
        return false;
    }
    _batteryCheckpointSequence = record.sequence;
    _batteryLastCheckpointMs = millis();
    _batteryDirty = false;
    _batteryLastError = "";
    Preferences prefs;
    if (_batteryNvsAvailable && prefs.begin("batenergy", false))
    {
        if (!_batteryLegacyInitialized)
        {
            _batteryLegacyInitialized = prefs.putBool("initialized", true) == 1;
            if (!_batteryLegacyInitialized)
                _batteryStorageWarning = "legacy_marker_write_failed";
        }
        if (_batteryFactoryResetPending)
        {
            if (prefs.remove("factory_reset"))
                _batteryFactoryResetPending = false;
            else
            {
                _batteryDirty = true;
                _batteryLastSaveOk = false;
                _batteryLastError = "factory_reset_finalize_failed";
            }
        }
        prefs.end();
    }
    return _batteryLastSaveOk;
}

void EnergyBacklog::publishBatteryEnergyState(SolarState &state)
{
    JsonObject live = state.doc()["LiveData"].as<JsonObject>();
    if (_batteryMigrationPending)
    {
        // Preserve HA's last retained totals until the old visible baseline is
        // carried over. A provisional zero would create an extra statistics reset.
        live.remove(DESCR_Battery_Charge_Energy);
        live.remove(DESCR_Battery_Discharge_Energy);
    }
    else
    {
        live[DESCR_Battery_Charge_Energy] = _batteryChargeWh / 1000.0;
        live[DESCR_Battery_Discharge_Energy] = _batteryDischargeWh / 1000.0;
    }
    state.refreshBindings();
    state.commitLocalState();
    _batteryLastStatePublishMs = millis();
    _batteryHaveStatePublish = true;
}

void EnergyBacklog::updateBatteryEnergy(SolarState &state, const BatterySampleTiming &sample, unsigned long nowMs)
{
    const uint32_t now = nowMs;
    if (!_batteryHaveIntegrationTime)
        advanceBatteryEnergy(now);
    if (static_cast<uint32_t>(now - _batteryLastIntegrateMs) > 0x7fffffffUL)
    {
        _batteryLastIntegrateMs = now;
        _batteryHaveSample = false;
        _batteryActivePowerW = 0;
    }
    const bool changed = sample.available && (!_batteryHaveSample ||
                         sample.sequence != _batterySample.sequence || sample.acceptedAtMs != _batterySample.acceptedAtMs);
    if (changed)
    {
        // Integrate the previous accepted reading until the actual transition,
        // including time spent waiting for UART. Never apply the new power to
        // the entire preceding interval or discard a sub-second phase.
        const uint32_t sinceCursor = sample.acceptedAtMs - static_cast<uint32_t>(_batteryLastIntegrateMs);
        const uint32_t untilNow = now - static_cast<uint32_t>(_batteryLastIntegrateMs);
        if (sinceCursor <= untilNow && untilNow <= 0x7fffffffUL)
            advanceBatteryEnergy(sample.acceptedAtMs);
        else
            advanceBatteryEnergy(now);
        JsonDocument snapshot;
        const bool completeSnapshot = state.snapshotTo(snapshot);
        const JsonObjectConst live = snapshot["LiveData"].as<JsonObjectConst>();
        const JsonVariantConst voltageValue = live[DESCR_Battery_Voltage];
        const JsonVariantConst charge = live[DESCR_Battery_Charge_Current];
        const JsonVariantConst discharge = live[DESCR_Battery_Discharge_Current];
        const JsonVariantConst net = live[DESCR_Battery_Load];
        const double voltage = voltageValue.isNull() ? 0 : voltageValue.as<double>();
        double current = 0;
        bool valid = completeSnapshot && voltageValue.is<double>() && std::isfinite(voltage) && voltage >= 5 && voltage <= 1000;
        if (!charge.isNull() && !discharge.isNull())
        {
            const double in = charge.as<double>(), out = discharge.as<double>();
            valid = valid && charge.is<double>() && discharge.is<double>() && std::isfinite(in) && std::isfinite(out) && in >= 0 && out >= 0 && in <= 1000 && out <= 1000;
            current = in - out;
        }
        else if (!net.isNull())
        {
            current = net.as<double>();
            valid = valid && net.is<double>();
        }
        else
            valid = false;
        valid = valid && std::isfinite(current) && current >= -1000 && current <= 1000 && sample.validForMs > 0 && sample.validForMs <= kMaxBatterySampleValidityMs &&
                static_cast<uint32_t>(now - sample.acceptedAtMs) <= 0x7fffffffUL &&
                static_cast<uint32_t>(sample.acceptedAtMs - sample.sampledAtMs) <= 0x7fffffffUL;
        _batteryMeasurementValid = valid;
        _batterySample = sample;
        // A failed response copy is retryable on the next tick. Do not consume
        // the new timestamp with an unavailable/partial set of power inputs.
        _batteryHaveSample = completeSnapshot;
        _batteryActivePowerW = valid && std::fabs(current) > 0.01 ? voltage * current : 0;
    }
    advanceBatteryEnergy(now);
    if (!sample.available)
        _batteryActivePowerW = 0;
    const bool fresh = sample.available && _batteryHaveSample &&
                       static_cast<uint32_t>(now - _batterySample.sampledAtMs) <= _batterySample.validForMs;
    const double power = fresh ? _batteryActivePowerW : 0;
    const int8_t direction = power > 0 ? 1 : power < 0 ? -1 : 0;
    const bool directionChanged = direction != _batteryDirection;
    _batteryDirection = direction;
    if (changed || directionChanged || !_batteryHaveStatePublish ||
        static_cast<uint32_t>(now - _batteryLastStatePublishMs) >= BATTERY_PUBLISH_INTERVAL_MS)
    {
        JsonObject live = state.doc()["LiveData"].as<JsonObject>();
        live[DESCR_Battery_Charging_Power_Calculated] = power > 0 ? power : 0;
        live[DESCR_Battery_Discharging_Power_Calculated] = power < 0 ? -power : 0;
        publishBatteryEnergyState(state);
    }
    // Keep flash traffic bounded even if direction changes several times per
    // second. Explicit restart/migration flushes are handled separately.
    if (_batteryDirty && static_cast<uint32_t>(now - _batteryLastCheckpointMs) >= BATTERY_CHECKPOINT_INTERVAL_MS &&
        (!_batteryHaveCheckpointAttempt || static_cast<uint32_t>(now - _batteryLastCheckpointAttemptMs) >= BATTERY_CHECKPOINT_RETRY_MS))
        saveBatteryEnergyCheckpoint(false);
}

bool EnergyBacklog::flushBatteryEnergy()
{
    advanceBatteryEnergy(millis());
    return saveBatteryEnergyCheckpoint(false);
}

bool EnergyBacklog::startReplay()
{
    if (!_ready)
    {
        return false;
    }

    repairTrailingPartialRecord();

    File file = LittleFS.open(BACKLOG_FILE, "r");
    if (!file)
    {
        return false;
    }

    const size_t size = file.size();
    file.close();

    if (size < sizeof(EnergyRecord))
    {
        return false;
    }

    _replayOffset = 0;
    _replayActive = true;
    writeLog("[EnergyBacklog] Replay started: %u bytes", static_cast<unsigned>(size));
    return true;
}

EnergyBacklog::ReplayResult EnergyBacklog::replayBatch(PubSubClient &client,
                                                       const String &baseTopic,
                                                       size_t maxRecords)
{
    if (!_replayActive)
    {
        return ReplayResult::Nothing;
    }

    if (!client.connected())
    {
        cancelReplay();
        return ReplayResult::Failed;
    }

    File file = LittleFS.open(BACKLOG_FILE, "r");
    if (!file)
    {
        cancelReplay();
        return ReplayResult::Failed;
    }

    const size_t fileSize = file.size();
    if (_replayOffset >= fileSize)
    {
        file.close();
        LittleFS.remove(BACKLOG_FILE);
        cancelReplay();
        return ReplayResult::Complete;
    }

    if (!file.seek(_replayOffset, SeekSet))
    {
        file.close();
        cancelReplay();
        return ReplayResult::Failed;
    }

    size_t processed = 0;
    EnergyRecord record{};

    while (processed < maxRecords &&
           file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record)) == sizeof(record))
    {
        _replayOffset += sizeof(record);

        if (!recordValid(record))
        {
            ++processed;
            continue;
        }

        if (!publishEnergyRecord(client, baseTopic, record))
        {
            file.close();
            cancelReplay();
            return ReplayResult::Failed;
        }

        ++processed;
    }

    const bool complete = _replayOffset >= fileSize;
    file.close();

    if (!complete)
    {
        return ReplayResult::InProgress;
    }

    if (!LittleFS.remove(BACKLOG_FILE))
    {
        writeLog("[EnergyBacklog] Replay complete but backlog delete failed");
        cancelReplay();
        return ReplayResult::Failed;
    }

    writeLog("[EnergyBacklog] Replay complete; backlog cleared");
    cancelReplay();
    return ReplayResult::Complete;
}

void EnergyBacklog::cancelReplay()
{
    _replayActive = false;
    _replayOffset = 0;
}

void EnergyBacklog::inspectBatteryPartitions()
{
    _batteryPartitions.clear();
    appendEnergyPartition(_batteryPartitions["runningApp"].to<JsonObject>(), esp_ota_get_running_partition());
    appendEnergyPartition(_batteryPartitions["nextOtaApp"].to<JsonObject>(), esp_ota_get_next_update_partition(nullptr));
    appendEnergyPartition(_batteryPartitions["backlog"].to<JsonObject>(), esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, BACKLOG_PARTITION));
    appendEnergyPartition(_batteryPartitions["nvs"].to<JsonObject>(), esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "nvs"));
}

void EnergyBacklog::advanceBatteryEnergy(uint32_t untilMs)
{
    if (!_batteryHaveIntegrationTime)
    {
        _batteryLastIntegrateMs = untilMs;
        _batteryHaveIntegrationTime = true;
        return;
    }
    const uint32_t elapsed = untilMs - static_cast<uint32_t>(_batteryLastIntegrateMs);
    if (elapsed > 0x7fffffffUL)
        return;
    if (_batteryHaveSample && _batteryActivePowerW != 0)
    {
        const uint32_t age = static_cast<uint32_t>(_batteryLastIntegrateMs) - _batterySample.sampledAtMs;
        const uint32_t remaining = age < _batterySample.validForMs ? _batterySample.validForMs - age : 0;
        const uint32_t validElapsed = elapsed < remaining ? elapsed : remaining;
        const double energyWh = std::fabs(_batteryActivePowerW) * static_cast<double>(validElapsed) / 3600000.0;
        if (energyWh > 0)
        {
            if (_batteryActivePowerW > 0)
                _batteryChargeWh += energyWh;
            else
                _batteryDischargeWh += energyWh;
            _batteryDirty = true;
        }
    }
    _batteryLastIntegrateMs = untilMs;
}

String EnergyBacklog::batteryMigrationReceipt() const
{
    char text[33];
    const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(_batteryMigrationId); ++i)
    {
        text[i * 2] = hex[_batteryMigrationId[i] >> 4];
        text[i * 2 + 1] = hex[_batteryMigrationId[i] & 15];
    }
    text[32] = '\0';
    return String(text);
}

bool EnergyBacklog::applyBatteryMigration(const char *requestId, double chargeKwh,
                                         double dischargeKwh, unsigned long nowMs, String &error)
{
    error = "";
    uint8_t id[16];
    if (!parseBatteryMigrationId(requestId, id) || !std::isfinite(chargeKwh) || !std::isfinite(dischargeKwh) ||
        chargeKwh < 0 || dischargeKwh < 0 || chargeKwh > BATTERY_MAX_COUNTER_WH / 1000 || dischargeKwh > BATTERY_MAX_COUNTER_WH / 1000)
    {
        error = "invalid_migration";
        return false;
    }
    if (!_batteryMigrationPending)
    {
        if (memcmp(id, _batteryMigrationId, sizeof(id)) == 0)
            return true;
        error = "already_initialized";
        return false;
    }
    if (_batteryMigrationUncertain && (_batteryUncertainRequest != requestId ||
        _batteryUncertainChargeKwh != chargeKwh || _batteryUncertainDischargeKwh != dischargeKwh))
    {
        error = "retry_same_migration_request";
        return false;
    }
    advanceBatteryEnergy(nowMs);
    const double oldCharge = _batteryChargeWh, oldDischarge = _batteryDischargeWh;
    const bool recovery = _batteryRecoveryRequired;
    if (oldCharge + chargeKwh * 1000 > BATTERY_MAX_COUNTER_WH || oldDischarge + dischargeKwh * 1000 > BATTERY_MAX_COUNTER_WH)
    {
        error = "counter_out_of_range";
        return false;
    }
    _batteryChargeWh += chargeKwh * 1000;
    _batteryDischargeWh += dischargeKwh * 1000;
    memcpy(_batteryMigrationId, id, sizeof(id));
    _batteryMigrationPending = _batteryMigrationUncertain = _batteryRecoveryRequired = false;
    _batteryDirty = true;
    if (saveBatteryEnergyCheckpoint(true))
    {
        _batteryUncertainRequest = "";
        _batteryHaveStatePublish = false;
        return true;
    }
    _batteryChargeWh = oldCharge;
    _batteryDischargeWh = oldDischarge;
    memset(_batteryMigrationId, 0, sizeof(_batteryMigrationId));
    _batteryMigrationPending = true;
    _batteryRecoveryRequired = recovery;
    _batteryMigrationUncertain = true;
    _batteryUncertainRequest = requestId;
    _batteryUncertainChargeKwh = chargeKwh;
    _batteryUncertainDischargeKwh = dischargeKwh;
    _batteryDirty = true;
    error = "migration_checkpoint_not_verified";
    return false;
}

void EnergyBacklog::writeBatteryDiagnostics(JsonObject target, unsigned long nowMs) const
{
    target["schema"] = 1;
    target["uptimeMs"] = static_cast<uint32_t>(nowMs);
    target["partitions"] = _batteryPartitions.as<JsonObjectConst>();
    JsonObject storage = target["storage"].to<JsonObject>();
    storage["backend"] = _batteryStore == BatteryStore::LittleFs ? "littlefs" : _batteryStore == BatteryStore::Nvs ? "nvs" : "none";
    storage["available"] = _batteryStore != BatteryStore::None;
    storage["filesystemMounted"] = _ready;
    storage["nvsAvailable"] = _batteryNvsAvailable;
    storage["loadedCheckpoint"] = _batteryLoadedCheckpoint;
    storage["sequence"] = _batteryCheckpointSequence;
    storage["lastWriteOk"] = _batteryLastSaveOk;
    storage["lastWriteAtMs"] = static_cast<uint32_t>(_batteryLastCheckpointMs);
    storage["dirty"] = _batteryDirty;
    storage["warning"] = _batteryStorageWarning;
    storage["error"] = _batteryLastError;
    JsonObject migration = target["migration"].to<JsonObject>();
    migration["pending"] = _batteryMigrationPending;
    migration["recoveryRequired"] = _batteryRecoveryRequired;
    migration["writeUncertain"] = _batteryMigrationUncertain;
    migration["factoryResetPending"] = _batteryFactoryResetPending;
    migration["receipt"] = batteryMigrationReceipt();
    migration["uncertainRequest"] = _batteryUncertainRequest;
    target["chargeKwh"] = _batteryChargeWh / 1000.0;
    target["dischargeKwh"] = _batteryDischargeWh / 1000.0;
    JsonObject integration = target["integration"].to<JsonObject>();
    integration["lastAcceptedPowerW"] = _batteryActivePowerW;
    integration["powerW"] = _batteryHaveSample && static_cast<uint32_t>(nowMs - _batterySample.sampledAtMs) <= _batterySample.validForMs ? _batteryActivePowerW : 0;
    integration["measurementValid"] = _batteryMeasurementValid;
    integration["direction"] = _batteryDirection;
    integration["lastIntegratedAtMs"] = static_cast<uint32_t>(_batteryLastIntegrateMs);
    integration["sampleSequence"] = _batterySample.sequence;
    integration["sampledAtMs"] = _batterySample.sampledAtMs;
    integration["acceptedAtMs"] = _batterySample.acceptedAtMs;
    integration["validForMs"] = _batterySample.validForMs;
}

bool EnergyBacklog::validBatteryMigrationId(const char *text)
{
    uint8_t id[16];
    return parseBatteryMigrationId(text, id);
}
