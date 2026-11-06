#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "solar/SolarTypes.h"

class PubSubClient;
class SolarState;

class EnergyBacklog
{
public:
    enum class ReplayResult : uint8_t
    {
        Nothing,
        InProgress,
        Complete,
        Failed,
    };

    EnergyBacklog();

    void begin();
    void captureIfNeeded(SolarState &state, bool mqttOffline, unsigned long nowMs);
    void updateBatteryEnergy(SolarState &state, const BatterySampleTiming &sample, unsigned long nowMs);
    bool flushBatteryEnergy();
    void writeBatteryDiagnostics(JsonObject target, unsigned long nowMs) const;
    bool batteryMigrationPending() const { return _batteryMigrationPending; }
    static bool validBatteryMigrationId(const char *text);
    String batteryMigrationReceipt() const;
    bool applyBatteryMigration(const char *requestId, double chargeKwh,
                               double dischargeKwh, unsigned long nowMs, String &error);
    bool startReplay();
    ReplayResult replayBatch(PubSubClient &client, const String &baseTopic, size_t maxRecords = 2);
    void cancelReplay();

private:
    bool _ready;
    bool _offlineActive;
    bool _haveLastValues;
    bool _replayActive;
    unsigned long _lastCaptureMs;
    uint32_t _nextSequence;
    size_t _replayOffset;
    uint16_t _lastMask;
    int64_t _lastValues[8];

    double _batteryChargeWh;
    double _batteryDischargeWh;
    unsigned long _batteryLastIntegrateMs;
    unsigned long _batteryLastCheckpointMs;
    uint32_t _batteryCheckpointSequence;
    int8_t _batteryDirection;
    bool _batteryDirty;
    enum class BatteryStore : uint8_t { None, LittleFs, Nvs };
    BatteryStore _batteryStore = BatteryStore::None;
    bool _batteryNvsAvailable = false;
    bool _batteryPreferNvs = false;
    bool _batteryMigrationPending = false;
    bool _batteryMigrationUncertain = false;
    bool _batteryFactoryResetPending = false;
    bool _batteryLoadedCheckpoint = false;
    bool _batteryLastSaveOk = false;
    bool _batteryHadRecords = false;
    bool _batteryLegacyInitialized = false;
    bool _batteryRecoveryRequired = false;
    int8_t _batteryFileSlot = -1;
    int8_t _batteryNvsSlot = -1;
    uint32_t _batteryWriteSequence = 0;
    uint8_t _batteryMigrationId[16] = {};
    String _batteryUncertainRequest;
    double _batteryUncertainChargeKwh = 0;
    double _batteryUncertainDischargeKwh = 0;
    String _batteryLastError;
    String _batteryStorageWarning;
    JsonDocument _batteryPartitions;
    bool _batteryHaveIntegrationTime = false;
    bool _batteryHaveSample = false;
    BatterySampleTiming _batterySample;
    double _batteryActivePowerW = 0;
    bool _batteryMeasurementValid = false;
    uint32_t _batteryLastStatePublishMs = 0;
    bool _batteryHaveStatePublish = false;
    uint32_t _batteryLastCheckpointAttemptMs = 0;
    bool _batteryHaveCheckpointAttempt = false;

    bool appendCurrentSnapshot(SolarState &state, unsigned long nowMs, bool force);
    bool compactIfNeeded(size_t incomingBytes);
    bool repairTrailingPartialRecord();
    void loadBatteryEnergyCheckpoint();
    bool saveBatteryEnergyCheckpoint(bool force);
    void publishBatteryEnergyState(SolarState &state);
    void resetBatteryEnergyIfFactoryReset();
    void inspectBatteryPartitions();
    void advanceBatteryEnergy(uint32_t untilMs);
};
