#include <Arduino.h>
#include <cassert>
#include <vector>
#include "solar/HaDiscoveryCatalog.h"

uint32_t testNow = 10000;
FakeEsp ESP;
struct SettingsMock {
    bool enabled = false;
    bool mqttHAEnabled() const { return enabled; }
    const char *mqttUser() const { return ""; }
    const char *mqttPassword() const { return ""; }
    const char *mqttTriggerPath() const { return ""; }
} settings;
struct { SettingsMock &get = settings; } _settings;
// The MAC is only used for the MQTT client ID in the reconnect path.
#define ESP testEsp
struct { uint64_t getEfuseMac() const { return 123; } } testEsp;
void writeLog(const char *, ...) {}
String getHaDeviceId() { return "test-device"; }
struct PubSubClient {
    struct Message { String topic, payload; bool retained; };
    bool online = false;
    std::vector<Message> messages;
    int subscriptions = 0;
    bool connected() const { return online; }
    template<class... Args> bool connect(Args...) { online = true; return true; }
    bool publish(const char *topic, const char *payload, bool retained) {
        messages.push_back({topic, payload, retained}); return true;
    }
    bool subscribe(const char *) { ++subscriptions; return true; }
    void unsubscribe(const char *) { --subscriptions; }
    void loop() {}
    void disconnect() { online = false; }
};
bool parseOwnHaDiscoveryTopic(const String &topic, const String &, String &, String &) {
    return topic.startsWith("homeassistant/");
}
bool isApprovedHaDiscoveryKey(const char *, const char *, bool) { return false; }
struct State {
    JsonDocument doc;
    void snapshotTo(JsonDocument &out) { out.set(doc); }
};
struct EnergyBacklog {
    enum class ReplayResult { InProgress, Complete, Failed };
    void updateBatteryEnergy(State &, bool, unsigned long) {}
    void captureIfNeeded(State &, bool, unsigned long) {}
    void cancelReplay() {}
    bool startReplay() { return false; }
    ReplayResult replayBatch(PubSubClient &, const String &, int) { return ReplayResult::Complete; }
};
struct MqttHandler {
    PubSubClient _mqtt;
    State _state;
    struct { void queueCommand(const String &) {} bool isConnected() const { return true; } } _inverterService;
    struct { bool getConnectionState() const { return true; } } _wifiManager;
    EnergyBacklog _energyBacklog;
    unsigned long _lastAlivePublish = testNow;
    bool _configured = true, _lastConnected = false, _replayingEnergyBacklog = false;
    unsigned long _lastReconnectAttempt = 0, _lastStatePublish = 0;
    unsigned long _delayedHaDiscoveryAt = 0, _haDiscoverySweepStartedMs = 0;
    bool _pendingHaDiscovery = false, _forceHaDiscovery = false;
    bool _pendingDelayedHaDiscovery = false, _pendingLegacyDs18Cleanup = false;
    bool _pendingFullPublish = false, _haDiscoverySweepActive = false;
    bool _haDiscoverySweepPowMr = false;
    String _haDiscoverySweepTopic;
    std::vector<String> _haDiscoveryTopics;
    bool ensureConnected();
    void loop();
    void triggerHaDiscovery();
    void publishHaDiscovery(bool);
    void handleMessage(char *, uint8_t *, unsigned int);
    void startHaDiscoverySweep();
    void stopHaDiscoverySweep();
    bool usesImmediateStatePublishing() const { return true; }
    String baseTopic() const { return "solar/test"; }
    void setupSubscriptions() {}
    void publishAlive() { _mqtt.publish("solar/test/Alive", "true", true); }
    void triggerFullStatePublish() { _pendingFullPublish = true; }
    uint32_t statePublishIntervalMs() const { return 0; }
    void publishState() { _mqtt.publish("solar/test/LiveData/Voltage", "230", true); }
    void publishHaSection(JsonDocument &, const char *section, JsonObjectConst,
                          const HaEntityDescriptor *, size_t,
                          std::vector<String> &topics, bool) {
        const String topic = String("homeassistant/sensor/test-device/") + section + "/config";
        _mqtt.publish(topic.c_str(), "{\"name\":\"Test\"}", true);
        topics.push_back(topic);
    }
    void publishHaEspInternalTemperature(JsonDocument &, JsonObjectConst, std::vector<String> &, bool) {}
    void publishHaDs18b20(JsonDocument &, JsonObjectConst, std::vector<String> &, bool) {}
    void publishHaPowMrSettings(JsonDocument &, JsonObjectConst, std::vector<String> &, bool) {}
};
#include "ha_production.h"

size_t haMessages(const MqttHandler &handler, bool empty) {
    size_t count = 0;
    for (const auto &m : handler._mqtt.messages)
        if (m.topic.startsWith("homeassistant/") && m.payload.isEmpty() == empty) ++count;
    return count;
}
void assertSilent(const MqttHandler &handler) {
    assert(haMessages(handler, true) == 0);
    assert(haMessages(handler, false) == 0);
}
int main() {
    MqttHandler handler;
    handler._state.doc["Status"]["protocol"] = "MODBUS_POWMR";
    handler._state.doc["DeviceData"]["Uncatalogued_Debug"] = 1;
    handler._state.doc["LiveData"]["Uncatalogued_Live"] = 1;
    // Disabled from first connection: no configs, no empty retained cleanup.
    assert(handler.ensureConnected());
    assert(handler._mqtt.messages.size() == 1); // Normal availability still works.
    assert(handler._mqtt.subscriptions == 0);
    assert(!handler._pendingDelayedHaDiscovery);
    handler.triggerHaDiscovery();
    assert(!handler._pendingHaDiscovery && !handler._forceHaDiscovery);
    handler.publishHaDiscovery(true);
    handler.publishHaDiscovery(false);
    assertSilent(handler);
    // Reconnect with discovery enabled: cleanup and publication still work.
    settings.enabled = true;
    handler._mqtt.online = false;
    testNow += 6000;
    assert(handler.ensureConnected());
    assert(handler._mqtt.subscriptions == 1);
    assert(handler._pendingDelayedHaDiscovery && handler._pendingHaDiscovery);
    handler.publishHaDiscovery(true);
    assert(haMessages(handler, true) > 0);
    assert(haMessages(handler, false) > 0);
    // Disable with queued work and a retained cleanup reply in flight.
    settings.enabled = false;
    handler._mqtt.messages.clear();
    handler.publishHaDiscovery(true);
    handler.publishHaDiscovery(false);
    char topic[] = "homeassistant/sensor/test-device/obsolete/config";
    uint8_t payload[] = {'{', '}'};
    handler.handleMessage(topic, payload, sizeof payload);
    assertSilent(handler);
    handler._pendingFullPublish = true;
    handler.loop();
    assert(!handler._pendingHaDiscovery && !handler._forceHaDiscovery);
    assert(!handler._pendingDelayedHaDiscovery && !handler._haDiscoverySweepActive);
    assertSilent(handler);
    assert(handler._mqtt.subscriptions == 0);
    handler._mqtt.online = false;
    testNow += 6000;
    assert(handler.ensureConnected());
    assertSilent(handler);
    puts("HA discovery: disabled connect/reconnect, manual/queued publish and in-flight cleanup stay silent; enabled cleanup/configs pass.");
}
