// #define isDEBUG
#include "modbus.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace
{
bool parseStrictLong(const String &text, long &value)
{
    String normalized = text;
    normalized.trim();
    if (normalized.isEmpty())
        return false;

    errno = 0;
    char *end = nullptr;
    const char *start = normalized.c_str();
    const long parsed = strtol(start, &end, 10);
    if (errno == ERANGE || end == start || end == nullptr || *end != '\0')
        return false;

    value = parsed;
    return true;
}

bool parseStrictFloat(const String &text, float &value)
{
    String normalized = text;
    normalized.trim();
    if (normalized.isEmpty())
        return false;

    errno = 0;
    char *end = nullptr;
    const char *start = normalized.c_str();
    const float parsed = strtof(start, &end);
    if (errno == ERANGE || end == start || end == nullptr || *end != '\0' || !std::isfinite(parsed))
        return false;

    value = parsed;
    return true;
}

uint16_t swapPowMrWord(uint16_t value)
{
    return static_cast<uint16_t>((value >> 8) | (value << 8));
}

bool readPowMrSettingMirror(MODBUS_COM &com,
                            uint16_t controlRegister,
                            uint16_t &logicalValue,
                            uint8_t attempts = 3)
{
    // PowMr/Victor writes settings through 50xx control registers, while the
    // readable/current values are mirrored 481 registers lower (4536..4552).
    if (controlRegister < 5017 || controlRegister > 5033)
    {
        return false;
    }

    const uint16_t mirrorRegister = static_cast<uint16_t>(controlRegister - 481U);
    for (uint8_t attempt = 0; attempt < attempts; ++attempt)
    {
        if (attempt == 0)
        {
            delay(150);
        }
        else
        {
            delay(250);
        }

        uint16_t wireValue = 0xFFFF;
        com.clearReadCache();
        if (com.readHoldingBlock(mirrorRegister, 1, &wireValue, 1))
        {
            logicalValue = swapPowMrWord(wireValue);
            return true;
        }
    }

    return false;
}
} // namespace

 
//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

MODBUS::MODBUS(HardwareSerial *port, int rxPin, int txPin)
{
    my_serialIntf = port;
    _rxPin = rxPin;
    _txPin = txPin;
}

MODBUS::~MODBUS()
{
    delete device;
    device = nullptr;
}
 
bool MODBUS::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    //this->my_serialIntf->setTimeout(2000);
 
    return true;
}

void MODBUS::prepareRegisters()
{
    const modbus_register_t *registers_live = device->getLiveRegisters();
    const modbus_register_t *registers_static = device->getStaticRegisters();

    live_info = {
        .variant = &liveData,
        .registers = registers_live,
        .array_size = device->getLiveRegistersCount(),
        .curr_register = 0};
    static_info = {
        .variant = &staticData,
        .registers = registers_static,
        .array_size = device->getStaticRegistersCount(),
        .curr_register = 0};
    previousTime = millis();
}

void MODBUS::loop()
{
    if (device == nullptr)
    {
        return;
    }

    if (_powmrDumpRunning)
    {
        previousTime = millis();
        return;
    }

    if (millis() - previousTime < kCommandDelayMs)
    {
        return;
    }

    modbus_register_info_t *cur_info_registers = &live_info;
    if (requestStaticData)
    {
        cur_info_registers = &static_info;
    }
    switch (_mCom.parseModbusToJson(*cur_info_registers))
    {
    case READ_OK:
        connectionCounter = 0;
        break;
    case READ_FAIL:
        connectionCounter++;
        break;
    default:
        break;
    }

    connection = connectionCounter < MAX_CONNECTION_ATTEMPTS;
    if (_mCom.isAllRegistersRead(*cur_info_registers))
    {
        const bool completedLivePass = (cur_info_registers == &live_info);
        requestStaticData = false;

        if (completedLivePass && device != nullptr && device->getProtocol() == MODBUS_POWMR)
        {
            static_cast<PowMr *>(device)->pollPv2(_mCom, liveData);
            capturePowmrSocSample();
            _powmrLivePassCompleted = true;
        }

        if (requestCallback && !_powmrLivePassCompleted)
        {
            requestCallback();
        }
    }

    stepPowmrHunt();
    previousTime = millis();
}

void MODBUS::callback(std::function<void()> func)
{
    requestCallback = func;
}

void MODBUS::capturePowmrSocSample()
{
    const unsigned long now = millis();
    if (_powmrSocHistoryCount > 0 &&
        static_cast<unsigned long>(now - _powmrSocLastSampleMs) < kPowMrSocSampleIntervalMs)
    {
        return;
    }

    const JsonVariant socVar = liveData[DESCR_Battery_Percent];
    const JsonVariant voltageVar = liveData[DESCR_Battery_Voltage];
    const JsonVariant chargeVar = liveData[DESCR_Battery_Charge_Current];
    const JsonVariant dischargeVar = liveData[DESCR_Battery_Discharge_Current];

    if (socVar.isNull() || voltageVar.isNull() || chargeVar.isNull() || dischargeVar.isNull())
    {
        return;
    }

    const int soc = socVar.as<int>();
    const float voltage = voltageVar.as<float>();
    const int charge = chargeVar.as<int>();
    const int discharge = dischargeVar.as<int>();

    if (soc < 0 || soc > 100 || voltage < 35.0f || voltage > 70.0f ||
        charge < 0 || charge > 300 || discharge < 0 || discharge > 300)
    {
        return;
    }

    PowMrSocSample &sample = _powmrSocHistory[_powmrSocHistoryHead];
    sample.uptimeSeconds = now / 1000UL;
    sample.batteryDeciVolts = static_cast<uint16_t>(voltage * 10.0f + 0.5f);
    sample.socPercent = static_cast<uint8_t>(soc);
    sample.chargeAmps = static_cast<uint16_t>(charge);
    sample.dischargeAmps = static_cast<uint16_t>(discharge);
    sample.valid = true;

    _powmrSocHistoryHead = static_cast<uint8_t>((_powmrSocHistoryHead + 1) % kPowMrSocHistorySize);
    if (_powmrSocHistoryCount < kPowMrSocHistorySize)
    {
        _powmrSocHistoryCount++;
    }
    _powmrSocLastSampleMs = now;
}

String MODBUS::buildPowmrSocDiag() const
{
    String answer;
    answer.reserve(7000);
    answer = "POWMR_SOC_DIAG interval=5min samples=";
    answer += static_cast<unsigned int>(_powmrSocHistoryCount);
    answer += "/";
    answer += static_cast<unsigned int>(kPowMrSocHistorySize);
    answer += "\n";

    if (_powmrSocHistoryCount == 0)
    {
        answer += "NO HISTORY YET: wait for a complete PowMr live-data pass";
        return answer;
    }

    const uint32_t nowSec = millis() / 1000UL;
    const uint8_t oldestIndex = static_cast<uint8_t>(
        (_powmrSocHistoryHead + kPowMrSocHistorySize - _powmrSocHistoryCount) %
        kPowMrSocHistorySize);

    uint8_t firstSoc = 0;
    uint8_t lastSoc = 0;
    uint32_t firstTime = 0;
    uint32_t lastTime = 0;
    float estimatedDischargeAh = 0.0f;
    bool first = true;
    bool havePrevious = false;
    PowMrSocSample previous;

    for (uint8_t i = 0; i < _powmrSocHistoryCount; ++i)
    {
        const uint8_t idx = static_cast<uint8_t>((oldestIndex + i) % kPowMrSocHistorySize);
        const PowMrSocSample &sample = _powmrSocHistory[idx];
        if (!sample.valid)
            continue;

        const uint32_t ageSec = nowSec >= sample.uptimeSeconds ? nowSec - sample.uptimeSeconds : 0;
        const uint32_t ageMin = ageSec / 60UL;

        char line[96];
        snprintf(line, sizeof(line),
                 "age=%lum SOC=%u V=%u.%u charge=%uA discharge=%uA\n",
                 static_cast<unsigned long>(ageMin),
                 static_cast<unsigned int>(sample.socPercent),
                 static_cast<unsigned int>(sample.batteryDeciVolts / 10),
                 static_cast<unsigned int>(sample.batteryDeciVolts % 10),
                 static_cast<unsigned int>(sample.chargeAmps),
                 static_cast<unsigned int>(sample.dischargeAmps));
        answer += line;

        if (first)
        {
            firstSoc = sample.socPercent;
            firstTime = sample.uptimeSeconds;
            first = false;
        }
        lastSoc = sample.socPercent;
        lastTime = sample.uptimeSeconds;

        if (havePrevious && sample.uptimeSeconds >= previous.uptimeSeconds)
        {
            const float hours = static_cast<float>(sample.uptimeSeconds - previous.uptimeSeconds) / 3600.0f;
            const float netDischarge = static_cast<float>(
                previous.dischargeAmps > previous.chargeAmps
                    ? previous.dischargeAmps - previous.chargeAmps
                    : 0);
            estimatedDischargeAh += netDischarge * hours;
        }
        previous = sample;
        havePrevious = true;
    }

    const uint32_t spanMin = (lastTime >= firstTime) ? (lastTime - firstTime) / 60UL : 0;
    answer += "SUMMARY span=";
    answer += static_cast<unsigned long>(spanMin);
    answer += "m SOC=";
    answer += static_cast<unsigned int>(firstSoc);
    answer += "->";
    answer += static_cast<unsigned int>(lastSoc);
    answer += " estimated_net_discharge=";
    answer += String(estimatedDischargeAh, 1);
    answer += "Ah";

    if (spanMin >= 30 && firstSoc == lastSoc && estimatedDischargeAh >= 5.0f)
    {
        answer += " WARNING=SOC_STUCK_SUSPECTED";
    }

    return answer;
}

void MODBUS::powmrDumpTask(void *param)
{
    MODBUS *self = static_cast<MODBUS *>(param);
    self->runPowmrDump();
    self->_powmrDumpTask = nullptr;
    vTaskDelete(nullptr);
}

void MODBUS::runPowmrDump()
{
    String answer;
    answer.reserve(22000);
    if (_powmrCustomScan)
    {
        answer = "POWMR_SCAN BEGIN holding=";
        answer += static_cast<unsigned int>(_powmrScanStart);
        answer += "-";
        answer += static_cast<unsigned int>(_powmrScanEnd);
        answer += " readable-only\n";
    }
    else
    {
        answer = "POWMR_DIAG BEGIN holding=4500-4564,5000-5038 readable-only\n";
    }

    uint16_t readable = 0;
    uint16_t failed = 0;
    const uint16_t oldTimeout = _mCom.getResponseTimeout();
    _mCom.setResponseTimeout(250);
    _mCom.clearReadCache();

    _powmrDumpCurrentRegister = 4500;
    _powmrDumpReadable = 0;
    _powmrDumpFailed = 0;

    ModbusMaster *mb = _mCom.getModbusMaster();

    auto appendReadable = [&](const char *space, uint16_t reg, uint16_t raw) {
        const uint16_t swapped = static_cast<uint16_t>((raw >> 8) | (raw << 8));
        const uint16_t candidates[] = {25, 30, 35, 70, 99, 250, 300, 350, 700, 990};
        bool marked = false;
        for (uint16_t v : candidates)
        {
            if (raw == v || swapped == v)
            {
                marked = true;
                break;
            }
        }
        char line[104];
        snprintf(line, sizeof(line), "%s reg=%u raw=%u swap=%u hex=0x%04X%s\n",
                 space, static_cast<unsigned int>(reg), static_cast<unsigned int>(raw),
                 static_cast<unsigned int>(swapped), static_cast<unsigned int>(raw),
                 marked ? " CANDIDATE" : "");
        answer += line;
        readable++;
        _powmrDumpReadable = readable;
    };

    auto scanHolding = [&](uint16_t first, uint16_t last) {
        for (uint16_t reg = first; reg <= last; ++reg)
        {
            _powmrDumpCurrentRegister = reg;
            const uint8_t result = mb->readHoldingRegisters(reg, 1);
            if (result == mb->ku8MBSuccess)
                appendReadable("H", reg, mb->getResponseBuffer(0));
            else
            {
                failed++;
                _powmrDumpFailed = failed;
            }
            vTaskDelay(1);
        }
    };

    if (_powmrCustomScan)
    {
        scanHolding(_powmrScanStart, _powmrScanEnd);
    }
    else
    {
        scanHolding(4500, 4564);
        scanHolding(5000, 5038);
    }

    answer += _powmrCustomScan ? "POWMR_SCAN END readable=" : "POWMR_DIAG END readable=";
    answer += readable;
    answer += " failed=";
    answer += failed;

    _mCom.setResponseTimeout(oldTimeout);
    _mCom.clearReadCache();

    _powmrDumpResult = answer;
    _powmrDumpReady = true;
    _powmrCustomScan = false;
    _powmrDumpRunning = false;

    writeLog("POWMR_DIAG complete readable=%u failed=%u",
             static_cast<unsigned int>(readable),
             static_cast<unsigned int>(failed));
}
String MODBUS::requestData(String command)
{
    command.trim();
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command.startsWith("powmr hunt"))
        return powmrHuntCommand(command);
    if (_powmrHunt.running && (command == "powmr dump" || command.startsWith("powmr scan ")))
        return "ERROR: stop hunt before starting another diagnostic scan";
    requestStaticData = true;

    // In-RAM SOC history sampled every five minutes from PowMr live registers.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command == "powmr socdiag")
    {
        return buildPowmrSocDiag();
    }

    // Compact read-only snapshot of known battery/BMS-facing registers.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command == "powmr bmsdiag")
    {
        if (_powmrDumpRunning)
            return "ERROR: wait for PowMr diagnostic scan to finish";

        auto readOne = [&](uint16_t reg, uint16_t &value) -> bool {
            _mCom.clearReadCache();
            return _mCom.readHoldingBlock(reg, 1, &value, 1);
        };
        auto swap16 = [](uint16_t value) -> uint16_t {
            return static_cast<uint16_t>((value >> 8) | (value << 8));
        };

        uint16_t r4506=0, r4507=0, r4508=0, r4509=0;
        uint16_t r4539=0, r4553=0, r4554=0, r4557=0;
        const bool ok =
            readOne(4506, r4506) && readOne(4507, r4507) &&
            readOne(4508, r4508) && readOne(4509, r4509) &&
            readOne(4539, r4539) && readOne(4553, r4553) &&
            readOne(4554, r4554) && readOne(4557, r4557);

        if (!ok)
            return "ERROR: one or more BMS diagnostic registers could not be read";

        String answer = "POWMR_BMS_DIAG";
        answer += " V=";
        answer += String(swap16(r4506) / 10.0f, 1);
        answer += " SOC4507=";
        answer += static_cast<unsigned int>(swap16(r4507));
        answer += " charge=";
        answer += static_cast<unsigned int>(swap16(r4508));
        answer += "A discharge=";
        answer += static_cast<unsigned int>(swap16(r4509));
        answer += "A batteryType=";
        answer += static_cast<unsigned int>(swap16(r4539));
        answer += " flags4553=0x";
        answer += String(swap16(r4553), HEX);
        answer += " flags4554=0x";
        answer += String(swap16(r4554), HEX);
        answer += " invTemp4557=";
        answer += static_cast<unsigned int>(swap16(r4557));
        answer += "C";
        return answer;
    }

    // Correlation helper for unknown PowMr registers.
    // First call stores a baseline; later calls show only changed registers.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        (command == "powmr watch" || command == "powmr watch reset"))
    {
        if (_powmrDumpRunning)
            return "ERROR: wait for PowMr diagnostic scan to finish";

        // Read all 64 registers from 4501 through 4564, including settings.
        // Keep the existing live/status requests and add the missing settings
        // block separately, without increasing the maximum request length.
        uint16_t current[kPowMrWatchCount] = {};
        _mCom.clearReadCache();
        const bool okA = _mCom.readHoldingBlock(4501, 34, current, 34);
        _mCom.clearReadCache();
        const bool okSettings = _mCom.readHoldingBlock(4535, 18, current + 34, 18);
        _mCom.clearReadCache();
        const bool okB = _mCom.readHoldingBlock(4553, 12, current + 52, 12);
        if (!okA || !okSettings || !okB)
            return "ERROR: unable to read one of the watch ranges; baseline unchanged";

        auto regForIndex = [](uint8_t i) -> uint16_t {
            return static_cast<uint16_t>(4501 + i);
        };
        auto swap16 = [](uint16_t value) -> uint16_t {
            return static_cast<uint16_t>((value >> 8) | (value << 8));
        };

        String answer;
        answer.reserve(5000);
        const uint32_t nowSec = millis() / 1000UL;

        if (!_powmrWatchValid || command == "powmr watch reset")
        {
            answer = "POWMR_WATCH BASELINE\n";
            for (uint8_t i = 0; i < kPowMrWatchCount; ++i)
            {
                const uint16_t reg = regForIndex(i);
                answer += String(reg);
                answer += " raw=";
                answer += static_cast<unsigned int>(current[i]);
                answer += " swap=";
                answer += static_cast<unsigned int>(swap16(current[i]));
                answer += "\n";
                _powmrWatchValues[i] = current[i];
            }
            _powmrWatchValid = true;
            _powmrWatchCapturedAt = nowSec;
            answer += "Fixed baseline. Run 'powmr watch' to compare; 'powmr watch reset' to replace baseline.";
            return answer;
        }

        const uint32_t ageSec = nowSec >= _powmrWatchCapturedAt ? nowSec - _powmrWatchCapturedAt : 0;
        answer = "POWMR_WATCH CHANGES since=";
        answer += static_cast<unsigned long>(ageSec);
        answer += "s\n";

        uint8_t changed = 0;
        for (uint8_t i = 0; i < kPowMrWatchCount; ++i)
        {
            if (current[i] == _powmrWatchValues[i])
                continue;

            const uint16_t reg = regForIndex(i);
            answer += String(reg);
            answer += " raw ";
            answer += static_cast<unsigned int>(_powmrWatchValues[i]);
            answer += "->";
            answer += static_cast<unsigned int>(current[i]);
            answer += " swap ";
            answer += static_cast<unsigned int>(swap16(_powmrWatchValues[i]));
            answer += "->";
            answer += static_cast<unsigned int>(swap16(current[i]));
            answer += "\n";
            changed++;
        }

        if (changed == 0)
            answer += "NO CHANGES\n";

        answer += "changed=";
        answer += static_cast<unsigned int>(changed);
        answer += "/";
        answer += static_cast<unsigned int>(kPowMrWatchCount);

        answer += "\nBaseline unchanged; use powmr watch reset to replace it.";
        return answer;
    }


    // PowMr/Victor output source priority (menu 01).
    // Read:  powmr outputmode
    // Write: powmr outputmode UTI|SUB|SBU
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        (command == "powmr outputmode" || command.startsWith("powmr outputmode ")))
    {
        if (_powmrDumpRunning)
            return "ERROR: wait for PowMr diagnostic scan to finish before changing output mode";

        auto modeName = [](uint16_t value) -> const char * {
            switch (value)
            {
            case 0: return "UTI";
            case 1: return "SUB";
            case 2: return "SBU";
            default: return "UNKNOWN";
            }
        };

        if (command == "powmr outputmode")
        {
            uint16_t value = 0xFFFF;
            if (!readPowMrSettingMirror(_mCom, 5018, value))
                return "ERROR: unable to read output mode mirror register 4537";

            return String("OK: Output mode = ") + modeName(value) +
                   " (" + static_cast<unsigned int>(value) + ")";
        }

        String requested = command.substring(17);
        requested.trim();
        requested.toUpperCase();

        int value = -1;
        if (requested == "UTI") value = 0;
        else if (requested == "SUB") value = 1;
        else if (requested == "SBU") value = 2;

        if (value < 0)
            return "ERROR: output mode must be UTI/SUB/SBU";

        if (!_mCom.writeHoldingRegister(5018, static_cast<uint16_t>(value)))
        {
            const uint8_t result = _mCom.getLastWriteResult();
            return String("ERROR: output mode write failed result=") +
                   static_cast<unsigned int>(result) + " (" +
                   _mCom.getLastWriteResultText() + ")";
        }

        uint16_t readback = 0xFFFF;
        const bool readOk = readPowMrSettingMirror(_mCom, 5018, readback);

        static_info.curr_register = 0;
        requestStaticData = true;

        if (!readOk)
            return String("OK: Output mode write accepted: ") + requested +
                   "; mirror readback pending";

        if (readback != static_cast<uint16_t>(value))
            return String("ERROR: output mode readback mismatch requested=") +
                   requested + " actual=" + modeName(readback) +
                   " (" + static_cast<unsigned int>(readback) + ")";

        return String("OK: Output mode = ") + modeName(readback) +
               " (" + static_cast<unsigned int>(readback) + ")";
    }

    // PowMr/Victor battery type (menu 05).
    // Read:  powmr batterytype
    // Write: powmr batterytype AGM|FLD|USE|LIB|LIC|LIP|LIL
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        (command == "powmr batterytype" || command.startsWith("powmr batterytype ")))
    {
        if (_powmrDumpRunning)
            return "ERROR: wait for PowMr scan to finish before changing battery type";

        auto batteryTypeName = [](uint16_t value) -> const char * {
            switch (value)
            {
            case 0: return "AGM";
            case 1: return "FLD";
            case 2: return "USE";
            case 3: return "LIB";
            case 4: return "LIC";
            case 5: return "LIP";
            case 6: return "LIL";
            default: return "UNKNOWN";
            }
        };

        if (command == "powmr batterytype")
        {
            uint16_t value = 0;
            if (!readPowMrSettingMirror(_mCom, 5020, value))
                return "ERROR: unable to read battery type mirror register 4539";

            return String("OK: Battery type = ") + batteryTypeName(value) +
                   " (" + static_cast<unsigned int>(value) + ")";
        }

        String requested = command.substring(18);
        requested.trim();
        requested.toUpperCase();

        int value = -1;
        if (requested == "AGM") value = 0;
        else if (requested == "FLD") value = 1;
        else if (requested == "USE") value = 2;
        else if (requested == "LIB") value = 3;
        else if (requested == "LIC") value = 4;
        else if (requested == "LIP") value = 5;
        else if (requested == "LIL") value = 6;

        if (value < 0)
            return "ERROR: battery type must be AGM/FLD/USE/LIB/LIC/LIP/LIL";

        if (!_mCom.writeHoldingRegister(5020, static_cast<uint16_t>(value)))
        {
            const uint8_t result = _mCom.getLastWriteResult();
            return String("ERROR: battery type write failed result=") +
                   static_cast<unsigned int>(result) + " (" +
                   _mCom.getLastWriteResultText() + ")";
        }

        uint16_t readback = 0xFFFF;
        const bool readOk = readPowMrSettingMirror(_mCom, 5020, readback);

        static_info.curr_register = 0;
        requestStaticData = true;

        if (!readOk)
            return String("OK: Battery type write accepted: ") + requested +
                   "; mirror readback pending";

        if (readback != static_cast<uint16_t>(value))
            return String("ERROR: battery type readback mismatch requested=") +
                   requested + " actual=" + batteryTypeName(readback) +
                   " (" + static_cast<unsigned int>(readback) + ")";

        return String("OK: Battery type = ") + batteryTypeName(readback) +
               " (" + static_cast<unsigned int>(readback) + ")";
    }

    // Known PowMr/Victor writable settings mirrored in 50xx control registers; inputs are parsed strictly.
    // Syntax: powmr setting <name> <value>
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command.startsWith("powmr setting "))
    {
        if (_powmrDumpRunning)
            return "ERROR: wait for PowMr diagnostic scan to finish before changing settings";

        String args = command.substring(14);
        args.trim();
        const int split = args.indexOf(' ');
        if (split <= 0)
            return "ERROR: syntax powmr setting <name> <value>";

        String name = args.substring(0, split);
        String valueText = args.substring(split + 1);
        name.trim();
        name.toLowerCase();
        valueText.trim();

        uint16_t reg = 0;
        uint16_t raw = 0;
        String displayValue;

        auto parseIntRange = [&](int minValue, int maxValue, uint16_t targetReg, const char *unit) -> bool {
            long value = 0;
            if (!parseStrictLong(valueText, value) || value < minValue || value > maxValue)
                return false;
            reg = targetReg;
            raw = static_cast<uint16_t>(value);
            displayValue = String(value);
            if (unit && *unit)
            {
                displayValue += " ";
                displayValue += unit;
            }
            return true;
        };

        auto parseVoltage = [&](float minValue, float maxValue, uint16_t targetReg) -> bool {
            float value = 0.0f;
            if (!parseStrictFloat(valueText, value) || value < minValue || value > maxValue)
                return false;
            reg = targetReg;
            raw = static_cast<uint16_t>(lroundf(value * 10.0f));
            displayValue = String(raw / 10.0f, 1) + " V";
            return true;
        };

        bool valid = false;

        if (name == "chargerpriority")
        {
            String v = valueText;
            v.toUpperCase();
            if (v == "UTILITY") { reg = 5017; raw = 0; valid = true; }
            else if (v == "SOLAR") { reg = 5017; raw = 1; valid = true; }
            else if (v == "SOLAR_UTILITY") { reg = 5017; raw = 2; valid = true; }
            else if (v == "SOLAR_ONLY") { reg = 5017; raw = 3; valid = true; }
            displayValue = v;
        }
        else if (name == "inputrange")
        {
            String v = valueText;
            v.toUpperCase();
            if (v == "APL") { reg = 5019; raw = 0; valid = true; displayValue = "Appliances"; }
            else if (v == "UPS") { reg = 5019; raw = 1; valid = true; displayValue = "UPS"; }
        }
        else if (name == "outputfreq")
        {
            long hz = 0;
            if (parseStrictLong(valueText, hz) && (hz == 50 || hz == 60))
            {
                reg = 5021;
                raw = hz == 50 ? 0 : 1;
                valid = true;
                displayValue = String(hz) + " Hz";
            }
        }
        else if (name == "maxcharge")
            valid = parseIntRange(0, 120, 5022, "A");
        else if (name == "outputvoltage")
        {
            long volts = 0;
            if (parseStrictLong(valueText, volts) && (volts == 220 || volts == 230 || volts == 240))
            {
                reg = 5023;
                raw = static_cast<uint16_t>(volts);
                valid = true;
                displayValue = String(volts) + " V";
            }
        }
        else if (name == "utilitycharge")
            valid = parseIntRange(0, 120, 5024, "A");
        else if (name == "recharge")
            valid = parseVoltage(40.0f, 60.0f, 5025);
        else if (name == "redischarge")
            valid = parseVoltage(40.0f, 60.0f, 5026);
        else if (name == "bulk")
            valid = parseVoltage(40.0f, 60.0f, 5027);
        else if (name == "float")
            valid = parseVoltage(40.0f, 60.0f, 5028);
        else if (name == "cutoff")
            valid = parseVoltage(40.0f, 60.0f, 5029);
        else if (name == "equalizationvoltage")
            valid = parseVoltage(40.0f, 60.0f, 5030);
        else if (name == "equalizationtime")
            valid = parseIntRange(0, 999, 5031, "min");
        else if (name == "equalizationtimeout")
            valid = parseIntRange(0, 999, 5032, "min");
        else if (name == "equalizationinterval")
            valid = parseIntRange(0, 999, 5033, "day");

        if (!valid || reg == 0)
            return String("ERROR: invalid or unsupported PowMr setting: ") + name + "=" + valueText;

        if (!_mCom.writeHoldingRegister(reg, raw))
        {
            const uint8_t result = _mCom.getLastWriteResult();
            return String("ERROR: setting write failed reg=") + reg +
                   " result=" + static_cast<unsigned int>(result) + " (" +
                   _mCom.getLastWriteResultText() + ")";
        }

        static_info.curr_register = 0;
        requestStaticData = true;

        uint16_t readback = 0xFFFF;
        const bool readOk = readPowMrSettingMirror(_mCom, reg, readback);
        if (!readOk)
        {
            // The 50xx control register write was acknowledged. Some PowMr
            // firmware does not allow those control registers to be read back
            // immediately; the normal 45xx mirror refresh will verify it.
            return String("OK: ") + name + " = " + displayValue +
                   " write accepted (reg " + reg + "); mirror readback pending";
        }

        if (readback != raw)
            return String("ERROR: setting readback mismatch reg=") + reg +
                   " requested=" + raw + " actual=" + readback;

        return String("OK: ") + name + " = " + displayValue +
               " (reg " + reg + ", mirror verified, raw " + raw + ")";
    }

    // Guarded first write command for PowMr/Victor.
    // Syntax: powmr charge <amps>
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command.startsWith("powmr charge "))
    {
        const String valueText = command.substring(13);
        long amps = 0;
        if (!parseStrictLong(valueText, amps))
        {
            return "ERROR: charge current must be an integer";
        }
        const bool allowed = amps == 10 || amps == 20 || amps == 30 ||
                             amps == 40 || amps == 50 || amps == 60;
        if (!allowed)
        {
            return "ERROR: allowed charge current is 10/20/30/40/50/60 A";
        }

        // PowMr control register 5022 is written in normal Modbus word order.
        // 50 A is value 0x0032; do not byte-swap control-register writes.
        const uint16_t rawValue = static_cast<uint16_t>(amps);

        if (!_mCom.writeHoldingRegister(5022, rawValue))
        {
            const uint8_t result = _mCom.getLastWriteResult();
            return String("ERROR: Modbus write failed result=") +
                   static_cast<unsigned int>(result) + " (" +
                   _mCom.getLastWriteResultText() + ")";
        }

        _mCom.clearReadCache();
        static_info.curr_register = 0;
        requestStaticData = true;
        return String("OK: Max charging current = ") + amps + " A";
    }

    // Safe read-only Victor/PowMr register diagnostics.
    // Syntax: powmr read <start> <count>. Allow the known diagnostic window
    // 4500..5099; unsupported addresses are reported as X. Never writes.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command.startsWith("powmr read "))
    {
        String args = command.substring(11);
        args.trim();
        const int split = args.indexOf(' ');
        if (split <= 0)
            return "ERROR: syntax powmr read <start> <count>";
        long start = 0;
        long count = 0;
        if (!parseStrictLong(args.substring(0, split), start) ||
            !parseStrictLong(args.substring(split + 1), count))
            return "ERROR: powmr read start/count must be integers";
        if (start < 4500 || start > 5099 || count < 1 || count > 20 ||
            start + count - 1 > 5099)
            return "ERROR: diagnostic range must stay within 4500..5099, max 20 registers";

        uint16_t values[20] = {};
        String answer = "OK:";
        _mCom.clearReadCache();

        if (_mCom.readHoldingBlock(static_cast<uint16_t>(start),
                                   static_cast<uint16_t>(count),
                                   values, 20))
        {
            for (int i = 0; i < count; ++i)
            {
                answer += " ";
                answer += String(start + i);
                answer += "=";
                answer += String(values[i]);
            }
            return answer;
        }

        // Some Victor/PowMr firmware rejects a block when even one address in
        // it is unsupported. Fall back to one-register reads and return all
        // readable values instead of failing the whole command.
        answer = "PARTIAL:";
        uint16_t readable = 0;
        uint16_t failed = 0;
        for (int i = 0; i < count; ++i)
        {
            const uint16_t reg = static_cast<uint16_t>(start + i);
            uint16_t value = 0;
            _mCom.clearReadCache();
            if (_mCom.readHoldingBlock(reg, 1, &value, 1))
            {
                answer += " ";
                answer += String(reg);
                answer += "=";
                answer += String(value);
                readable++;
            }
            else
            {
                answer += " ";
                answer += String(reg);
                answer += "=X";
                failed++;
            }
        }

        answer += " [readable=";
        answer += readable;
        answer += " failed=";
        answer += failed;
        answer += "]";
        return answer;
    }

    // Read-only asynchronous PowMr/Victor extended holding-register scan.
    // Syntax: powmr scan <start> <end>; result: powmr scan result.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command == "powmr scan result")
    {
        if (_powmrDumpRunning)
        {
            return String("RUNNING: reg=") +
                   static_cast<unsigned int>(_powmrDumpCurrentRegister) +
                   " readable=" + static_cast<unsigned int>(_powmrDumpReadable) +
                   " failed=" + static_cast<unsigned int>(_powmrDumpFailed);
        }
        if (!_powmrDumpReady)
            return "NO RESULT: start with 'powmr scan <start> <end>'";
        return _powmrDumpResult;
    }

    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command.startsWith("powmr scan "))
    {
        if (_powmrDumpRunning)
            return "RUNNING: PowMr diagnostic scan already started";

        String args = command.substring(11);
        args.trim();
        const int split = args.indexOf(' ');
        if (split <= 0)
            return "ERROR: syntax powmr scan <start> <end>";

        long first = 0;
        long last = 0;
        if (!parseStrictLong(args.substring(0, split), first) ||
            !parseStrictLong(args.substring(split + 1), last))
            return "ERROR: powmr scan start/end must be integers";
        if (first < 4000 || last > 6000 || first > last || (last - first + 1) > 500)
            return "ERROR: scan range must stay within 4000..6000 and contain at most 500 registers";

        _powmrCustomScan = true;
        _powmrScanStart = static_cast<uint16_t>(first);
        _powmrScanEnd = static_cast<uint16_t>(last);
        _powmrDumpReady = false;
        _powmrDumpResult = "";
        _powmrDumpRunning = true;

        TaskHandle_t handle = nullptr;
        if (xTaskCreate(powmrDumpTask, "powmr_scan", 8192, this, 1, &handle) != pdPASS)
        {
            _powmrDumpRunning = false;
            _powmrCustomScan = false;
            return "ERROR: failed to start PowMr extended scan task";
        }

        _powmrDumpTask = handle;
        return String("STARTED: PowMr scan ") + first + "-" + last +
               "; use 'powmr scan result' for progress/result";
    }

    // Read-only asynchronous PowMr/Victor diagnostic dump.
    // Start with "powmr dump"; retrieve later with "powmr dump result".
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command == "powmr dump result")
    {
        if (_powmrDumpRunning)
        {
            return String("RUNNING: reg=") +
                   static_cast<unsigned int>(_powmrDumpCurrentRegister) +
                   " readable=" + static_cast<unsigned int>(_powmrDumpReadable) +
                   " failed=" + static_cast<unsigned int>(_powmrDumpFailed);
        }
        if (!_powmrDumpReady)
            return "NO RESULT: start with 'powmr dump'";
        return _powmrDumpResult;
    }

    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        command == "powmr dump")
    {
        if (_powmrDumpRunning)
            return "RUNNING: PowMr dump already started";

        _powmrDumpReady = false;
        _powmrDumpResult = "";
        _powmrDumpRunning = true;

        TaskHandle_t handle = nullptr;
        if (xTaskCreate(powmrDumpTask, "powmr_dump", 8192, this, 1, &handle) != pdPASS)
        {
            _powmrDumpRunning = false;
            return "ERROR: failed to start PowMr dump task";
        }

        _powmrDumpTask = handle;
        return "STARTED: PowMr BMS/SOC/temperature diagnostic scan; use 'powmr dump result' for progress/result";
    }

    // SOC threshold writes are intentionally disabled until the exact Victor
    // lithium/BMS registers and encoding are confirmed by read-only diagnostics.
    if (device != nullptr && device->getProtocol() == MODBUS_POWMR &&
        (command.startsWith("powmr backutility ") ||
         command.startsWith("powmr backbattery ")))
        return "ERROR: SBU SOC writes disabled until registers are verified";

    writeLog("Custom Modbus command unsupported: %s", command.c_str());
    return "UNSUPPORTED";
}

bool MODBUS::forceProtocol(protocol_type_t protocol)
{
    delete device;
    device = nullptr;

    switch (protocol)
    {
    case MODBUS_POWMR:
    case MODBUS_POWMR_PI:
        device = new PowMr();
        break;
    case MODBUS_DEYE:
        device = new Deye();
        break;
    case MODBUS_SMG_II_11KW:
        device = new SMGII11KW();
        break;
    case MODBUS_SMG:
        device = new SMG();
        break;
    case MODBUS_ANENJI_SRNE:
        device = new AnenjiSrne();
        break;
    case MODBUS_ANENJI:
        device = new Anenji();
        break;
    case MODBUS_MUST:
        device = new MustPV_PH18();
        break;
    default:
        return false;
    }

    device->init(*my_serialIntf, _rxPin, _txPin, _mCom, true);
    stabilizeSerial();
    _mCom.setResponseTimeout(device->getResponseTimeout());
    staticData[DESCR_Device_Model] = device->getName();
    staticData[DESCR_Protocol_ID] = (protocol == MODBUS_POWMR_PI) ? "PI+Modbus" : protocolToString(protocol);
    prepareRegisters();
    writeLog("Modbus protocol forced to %s", protocolToString(protocol));
    return true;
}

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
protocol_type_t MODBUS::autoDetect(bool powMrOnly) // function for autodetect the inverter type
{
    protocol_type_t protocol = NoD;
    char modelName[48] = {};
    long activeBaudRate = 0;
    const uint16_t normalResponseTimeout = _mCom.getResponseTimeout();

    writeLog("Try Autodetect Modbus device");
    _mCom.setResponseTimeout(MODBUS_DETECTION_TIMEOUT_MS);

    ModbusDevice *devices[] = { new PowMr(), new Deye(), new SMGII11KW(), new SMG(), new AnenjiSrne(), new Anenji(), new MustPV_PH18()};
    const size_t deviceCount = sizeof(devices) / sizeof(devices[0]);

    for (size_t i = 0; i < deviceCount; ++i)
    {
        modelName[0] = '\0';
        const bool configureSerial = activeBaudRate != devices[i]->getBaudRate();
        devices[i]->init(*my_serialIntf, _rxPin, _txPin, _mCom, configureSerial);
        if (configureSerial)
        {
            activeBaudRate = devices[i]->getBaudRate();
            stabilizeSerial();
        }

        const bool detected = devices[i]->retrieveModel(_mCom, modelName, sizeof(modelName));

        if (detected && modelName[0] != '\0')
        {
            writeLog("<Autodetect> Found Modbus device: %s", modelName);
            staticData["Device_Model"] = modelName;
            
            device = devices[i];
            prepareRegisters();
            protocol = device->getProtocol();
            staticData[DESCR_Protocol_ID] = protocolToString(protocol);

            // Clean up candidates that were not selected. Earlier failures are
            // already deleted and nulled below, so guard against double-free.
            for (size_t j = 0; j < deviceCount; ++j)
            {
                if (j != i && devices[j] != nullptr)
                {
                    delete devices[j];
                    devices[j] = nullptr;
                }
            }
            _mCom.setResponseTimeout(device->getResponseTimeout());
            return protocol;
        }
        delete devices[i];
        devices[i] = nullptr;

        if (powMrOnly)
        {
            break;
        }
    }

    for (size_t i = 0; i < deviceCount; ++i)
    {
        if (devices[i] != nullptr)
        {
            delete devices[i];
            devices[i] = nullptr;
        }
    }

    _mCom.setResponseTimeout(normalResponseTimeout);
    return protocol;
}

void MODBUS::stabilizeSerial()
{
    while (my_serialIntf->available() > 0)
    {
        my_serialIntf->read();
    }

    delay(kSerialStabilizationDelayMs);

    while (my_serialIntf->available() > 0)
    {
        my_serialIntf->read();
    }
}
