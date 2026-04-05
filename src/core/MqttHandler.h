#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <vector>

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

private:
    static MqttHandler *s_instance;

    SolarState &_state;
    WiFiManager &_wifiManager;
    SolarInverterService &_inverterService;

    WiFiClient _plainClient;
    WiFiClientSecure _secureClient;
    Client *_netClient;
    PubSubClient _mqtt;

    bool _pendingFullPublish;
    bool _pendingHaDiscovery;
    bool _forceHaDiscovery;
    bool _pendingLegacyDs18Cleanup;
    bool _configured;
    bool _lastConnected;
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
    void publishHaDs18b20(JsonDocument &snapshot, JsonObjectConst liveValues, std::vector<String> &currentTopics, bool force);
    bool hasHaDiscoveryTopic(const String &topic) const;
    void setupSubscriptions();
    String baseTopic() const;
};
