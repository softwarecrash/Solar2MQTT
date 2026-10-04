#pragma once

#include <Arduino.h>

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
    void updateBatteryEnergy(SolarState &state, bool inverterConnected, unsigned long nowMs);
    void flushBatteryEnergy();
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

    bool appendCurrentSnapshot(SolarState &state, unsigned long nowMs, bool force);
    bool compactIfNeeded(size_t incomingBytes);
    bool repairTrailingPartialRecord();
    void loadBatteryEnergyCheckpoint();
    bool saveBatteryEnergyCheckpoint(bool force);
    void publishBatteryEnergyState(SolarState &state);
    void resetBatteryEnergyIfFactoryReset();
};
