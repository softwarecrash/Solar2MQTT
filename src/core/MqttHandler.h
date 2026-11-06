#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <vector>
#include <freertos/semphr.h>

#include "core/EnergyBacklog.h"
#include "solar/HaDiscoveryCatalog.h"

class SolarState;
class WiFiManager;
class SolarInverterService;

class MqttHandler
{
public:
    MqttHandler(SolarState &state, WiFiManager &wifiManager, SolarInverterService &inverterService);

    void begin();
    void loop();
    void reconfigure();

    bool isConnected();
    void triggerFullStatePublish();
    void triggerHaDiscovery();
    void publishSensorImmediate(uint8_t index, float temperature);
    void flushPersistentEnergy();
    void copyBatteryDiagnostics(JsonDocument &target);
    int queueBatteryMigration(JsonObjectConst request, JsonDocument &response);

private:
    static MqttHandler *s_instance;

    SolarState &_state;
    WiFiManager &_wifiManager;
    SolarInverterService &_inverterService;

    WiFiClient _plainClient;
    WiFiClientSecure _secureClient;
    Client *_netClient;
    PubSubClient _mqtt;
    EnergyBacklog _energyBacklog;
    struct BatteryMigrationRequest
    {
        String id;
        double chargeKwh = 0;
        double dischargeKwh = 0;
    };
    // HTTP only reads this cache or queues a request. Inverter state, NVS and
    // filesystem mutations stay on the main loop, outside this short lock.
    class BatteryApiLock
    {
    public:
        explicit BatteryApiLock(SemaphoreHandle_t mutex) : _mutex(mutex) { if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY); }
        ~BatteryApiLock() { if (_mutex) xSemaphoreGive(_mutex); }
    private:
        SemaphoreHandle_t _mutex;
    };
    SemaphoreHandle_t _batteryApiMutex = nullptr;
    JsonDocument _batteryApiCache;
    String _batteryApiDeviceId;
    String _batteryApiBootId;
    BatteryMigrationRequest _batteryApiRequest;
    bool _batteryApiQueued = false;
    bool _batteryApiBusy = false;
    String _batteryJobId;
    String _batteryJobState;
    String _batteryJobError;
    uint32_t _batteryApiLastRefreshMs = 0;
    bool _batteryApiHaveRefresh = false;

    bool _pendingFullPublish;
    bool _pendingHaDiscovery;
    bool _forceHaDiscovery;
    bool _pendingLegacyDs18Cleanup;
    bool _configured;
    bool _lastConnected;
    bool _replayingEnergyBacklog;
    bool _haDiscoverySweepActive;
    bool _haDiscoverySweepPowMr;
    unsigned long _haDiscoverySweepStartedMs;
    String _haDiscoverySweepTopic;
    bool _pendingDelayedHaDiscovery;
    unsigned long _delayedHaDiscoveryAt;
    unsigned long _lastReconnectAttempt;
    unsigned long _lastAlivePublish;
    unsigned long _lastStatePublish;
    std::vector<String> _haDiscoveryTopics;

    static void globalCallback(char *topic, uint8_t *payload, unsigned int length);
    void handleMessage(char *topic, uint8_t *payload, unsigned int length);

    void configureClient();
    bool ensureConnected();
    uint32_t statePublishIntervalMs() const;
    bool usesImmediateStatePublishing() const;
    void publishAlive();
    void publishState();
    void publishFlatState(JsonDocument &snapshot);
    void publishJsonState(JsonDocument &snapshot);
    void publishObjectSection(const char *sectionName, JsonObjectConst object);
    void publishRawState(JsonObjectConst rawObject);
    void publishHaDiscovery(bool force);
    void publishHaSection(JsonDocument &snapshot,
                          const char *stateSection,
                          JsonObjectConst object,
                          const HaEntityDescriptor *descriptors,
                          size_t descriptorCount,
                          std::vector<String> &currentTopics,
                          bool force);
    void publishHaEspInternalTemperature(JsonDocument &snapshot, JsonObjectConst espValues, std::vector<String> &currentTopics, bool force);
    void publishHaDs18b20(JsonDocument &snapshot, JsonObjectConst liveValues, std::vector<String> &currentTopics, bool force);
    void publishHaPowMrSettings(JsonDocument &snapshot, JsonObjectConst deviceValues, std::vector<String> &currentTopics, bool force);
    bool hasHaDiscoveryTopic(const String &topic) const;
    void startHaDiscoverySweep();
    void stopHaDiscoverySweep();
    void setupSubscriptions();
    String baseTopic() const;
    void initializeBatteryApi(const String &deviceId, const String &bootId);
    void refreshBatteryDiagnostics(unsigned long nowMs, bool force = false);
    void processBatteryMigration(unsigned long nowMs);
    void preserveBatteryDiscoveryTopics(std::vector<String> &topics) const;
};
