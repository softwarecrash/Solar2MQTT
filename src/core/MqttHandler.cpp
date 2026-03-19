#include "core/MqttHandler.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "core/SettingsPrefs.h"
#include "core/SolarState.h"
#include "core/WiFiManager.h"
#include "solar/HaDiscoveryCatalog.h"
#include "solar/SolarInverterService.h"

extern Settings _settings;

namespace
{
const HaEntityDescriptor *findDescriptor(const char *name,
                                         const HaEntityDescriptor *descriptors,
                                         size_t descriptorCount)
{
    for (size_t i = 0; i < descriptorCount; ++i)
    {
        if (strcmp(descriptors[i].name, name) == 0)
        {
            return &descriptors[i];
        }
    }
    return nullptr;
}

bool containsTopic(const std::vector<String> &topics, const String &topic)
{
    for (const String &candidate : topics)
    {
        if (candidate == topic)
        {
            return true;
        }
    }
    return false;
}

void appendTopicIfMissing(std::vector<String> &topics, const String &topic)
{
    if (!containsTopic(topics, topic))
    {
        topics.push_back(topic);
    }
}

bool isDiscoverableValue(JsonVariantConst value)
{
    return !value.isNull() && !value.is<JsonObjectConst>() && !value.is<JsonArrayConst>();
}

String buildDiscoveryTopic(const String &baseTopic, const char *component, const char *key)
{
    return String("homeassistant/") + component + "/" + baseTopic + "/" + key + "/config";
}

String buildUniqueId(const char *baseTopic, const char *sectionName, const char *key)
{
    return String(baseTopic) + "." + sectionName + "." + key;
}

void populateDeviceInfo(JsonDocument &doc)
{
    JsonObject device = doc["dev"].to<JsonObject>();
    device["ids"][0] = _settings.get.deviceName();
    device["name"] = _settings.get.deviceName();
    device["sw"] = STRVERSION;
}

void publishJsonValue(PubSubClient &client, const String &topic, JsonVariantConst value, bool retained = true)
{
    String payload;
    if (value.is<bool>())
    {
        payload = value.as<bool>() ? "true" : "false";
    }
    else if (value.is<const char *>())
    {
        payload = value.as<const char *>();
    }
    else if (value.is<String>())
    {
        payload = value.as<String>();
    }
    else if (value.isNull())
    {
        payload = "";
    }
    else
    {
        serializeJson(value, payload);
    }
    client.publish(topic.c_str(), payload.c_str(), retained);
}
} // namespace

MqttHandler *MqttHandler::s_instance = nullptr;

MqttHandler::MqttHandler(SolarState &state, WiFiManager &wifiManager, SolarInverterService &inverterService)
    : _state(state),
      _wifiManager(wifiManager),
      _inverterService(inverterService),
      _netClient(&_plainClient),
      _mqtt(_plainClient),
      _pendingFullPublish(false),
      _pendingHaDiscovery(false),
      _forceHaDiscovery(false),
      _configured(false),
      _lastConnected(false),
      _lastReconnectAttempt(0),
      _lastAlivePublish(0),
      _lastStatePublish(0)
{
    s_instance = this;
}

void MqttHandler::begin()
{
    configureClient();
    _configured = strlen(_settings.get.mqttHost()) > 0;
    _lastReconnectAttempt = 0;
    _lastAlivePublish = millis();
    _lastStatePublish = millis();
    _lastConnected = false;
    _forceHaDiscovery = false;
    _haDiscoveryTopics.clear();
}

void MqttHandler::loop()
{
    if (!_configured || !_wifiManager.getConnectionState())
    {
        return;
    }

    const bool connected = ensureConnected();
    if (connected)
    {
        _mqtt.loop();
    }
    const bool inverterBusy = _inverterService.isBusy();

    const unsigned long now = millis();
    if (connected && (now - _lastAlivePublish) >= 30000UL)
    {
        _lastAlivePublish = now;
        publishAlive();
    }

    const uint32_t intervalMs = _settings.get.mqttRefresh() * 1000UL;
    if (connected && intervalMs > 0 && (now - _lastStatePublish) >= intervalMs)
    {
        _lastStatePublish = now;
        _pendingFullPublish = true;
    }

    if (connected && _pendingFullPublish && !inverterBusy)
    {
        _pendingFullPublish = false;
        publishState();

        if (_settings.get.mqttHAEnabled() && !_pendingHaDiscovery)
        {
            publishHaDiscovery(false);
        }
    }

    if (connected && _pendingHaDiscovery && !inverterBusy)
    {
        const bool force = _forceHaDiscovery;
        _pendingHaDiscovery = false;
        _forceHaDiscovery = false;
        publishHaDiscovery(force);
    }

    _lastConnected = connected;
}

bool MqttHandler::isConnected()
{
    return _mqtt.connected();
}

void MqttHandler::triggerHaDiscovery()
{
    _pendingHaDiscovery = true;
    _forceHaDiscovery = true;
}

void MqttHandler::publishSensorImmediate(uint8_t index, float temperature)
{
    if (!_mqtt.connected())
    {
        return;
    }

    char topic[64];
    snprintf(topic, sizeof(topic), "%s/DS18B20_%u", _settings.get.mqttTopic(), static_cast<unsigned>(index));
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", static_cast<double>(temperature));
    _mqtt.publish(topic, payload, true);
}

void MqttHandler::globalCallback(char *topic, uint8_t *payload, unsigned int length)
{
    if (s_instance != nullptr)
    {
        s_instance->handleMessage(topic, payload, length);
    }
}

void MqttHandler::handleMessage(char *topic, uint8_t *payload, unsigned int length)
{
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; ++i)
    {
        message += static_cast<char>(payload[i]);
    }

    const String topicString(topic);
    if (strlen(_settings.get.mqttTriggerPath()) > 0 && topicString == _settings.get.mqttTriggerPath())
    {
        _pendingFullPublish = true;
        return;
    }

    const String commandTopic = baseTopic() + "/DeviceControl/Set_Command";
    if (topicString == commandTopic)
    {
        _inverterService.queueCommand(message);
        _pendingFullPublish = true;
    }
}

void MqttHandler::configureClient()
{
    if (_settings.get.mqttSSL())
    {
        _secureClient.setInsecure();
        _secureClient.setHandshakeTimeout(30);
        _netClient = &_secureClient;
    }
    else
    {
        _netClient = &_plainClient;
    }

    _mqtt.setClient(*_netClient);
    _mqtt.setServer(_settings.get.mqttHost(), _settings.get.mqttPort());
    _mqtt.setCallback(MqttHandler::globalCallback);
    _mqtt.setBufferSize(4096);
    _mqtt.setKeepAlive(30);
}

bool MqttHandler::ensureConnected()
{
    if (_mqtt.connected())
    {
        return true;
    }

    if ((millis() - _lastReconnectAttempt) < 5000UL)
    {
        return false;
    }
    _lastReconnectAttempt = millis();

    uint64_t mac = ESP.getEfuseMac();
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "Solar2MQTT-%04X%08X",
             static_cast<uint16_t>(mac >> 32),
             static_cast<uint32_t>(mac));

    const String lwtTopic = baseTopic() + "/Alive";
    bool ok = false;
    if (strlen(_settings.get.mqttUser()) > 0)
    {
        ok = _mqtt.connect(clientId,
                           _settings.get.mqttUser(),
                           _settings.get.mqttPassword(),
                           lwtTopic.c_str(),
                           0,
                           true,
                           "false");
    }
    else
    {
        ok = _mqtt.connect(clientId, nullptr, nullptr, lwtTopic.c_str(), 0, true, "false");
    }

    if (!ok)
    {
        return false;
    }

    setupSubscriptions();
    publishAlive();
    _pendingFullPublish = true;
    if (_settings.get.mqttHAEnabled())
    {
        _pendingHaDiscovery = true;
        _forceHaDiscovery = true;
    }
    return true;
}

void MqttHandler::publishAlive()
{
    const String topic = baseTopic() + "/Alive";
    _mqtt.publish(topic.c_str(), "true", true);

    const String ipTopic = baseTopic() + "/IP";
    const String ip = _wifiManager.ipAddress();
    _mqtt.publish(ipTopic.c_str(), ip.c_str(), true);
}

void MqttHandler::publishState()
{
    JsonDocument snapshot;
    _state.snapshotTo(snapshot);

    if (_settings.get.mqttJson())
    {
        publishJsonState(snapshot);
        if (_settings.get.mqttHAEnabled())
        {
            publishFlatState(snapshot);
        }
    }
    else
    {
        publishFlatState(snapshot);
    }
}

void MqttHandler::publishFlatState(JsonDocument &snapshot)
{
    publishObjectSection("EspData", snapshot["EspData"].as<JsonObjectConst>());
    publishObjectSection("DeviceData", snapshot["DeviceData"].as<JsonObjectConst>());
    publishObjectSection("LiveData", snapshot["LiveData"].as<JsonObjectConst>());
    publishRawState(snapshot["RawData"].as<JsonObjectConst>());

    if (_inverterService.hasCommandAnswer())
    {
        const String topic = baseTopic() + "/DeviceControl/Set_Command_answer";
        const String answer = _inverterService.consumeCommandAnswer();
        _mqtt.publish(topic.c_str(), answer.c_str(), false);
    }
}

void MqttHandler::publishJsonState(JsonDocument &snapshot)
{
    const String topic = baseTopic() + "/Data";
    _mqtt.beginPublish(topic.c_str(), measureJson(snapshot), false);
    serializeJson(snapshot, _mqtt);
    _mqtt.endPublish();
}

void MqttHandler::publishObjectSection(const char *sectionName, JsonObjectConst object)
{
    for (JsonPairConst entry : object)
    {
        const String topic = baseTopic() + "/" + sectionName + "/" + entry.key().c_str();
        publishJsonValue(_mqtt, topic, entry.value(), true);

        if (strcmp(sectionName, "EspData") == 0 && strncmp(entry.key().c_str(), "DS18B20_", 8) == 0)
        {
            const String directTopic = baseTopic() + "/" + entry.key().c_str();
            publishJsonValue(_mqtt, directTopic, entry.value(), true);
        }
    }
}

void MqttHandler::publishRawState(JsonObjectConst rawObject)
{
    for (JsonPairConst entry : rawObject)
    {
        const String topic = baseTopic() + "/RAW/" + entry.key().c_str();
        publishJsonValue(_mqtt, topic, entry.value(), false);
    }
}

void MqttHandler::publishHaDiscovery(bool force)
{
    JsonDocument snapshot;
    _state.snapshotTo(snapshot);

    std::vector<String> currentTopics;
    currentTopics.reserve(_haDiscoveryTopics.size() + 16);

    publishHaSection("DeviceData",
                     snapshot["DeviceData"].as<JsonObjectConst>(),
                     HA_STATIC_DESCRIPTORS,
                     sizeof(HA_STATIC_DESCRIPTORS) / sizeof(HaEntityDescriptor),
                     currentTopics,
                     force);
    publishHaSection("LiveData",
                     snapshot["LiveData"].as<JsonObjectConst>(),
                     HA_LIVE_DESCRIPTORS,
                     sizeof(HA_LIVE_DESCRIPTORS) / sizeof(HaEntityDescriptor),
                     currentTopics,
                     force);
    publishHaDs18b20(snapshot["EspData"].as<JsonObjectConst>(), currentTopics, force);

    if (!force)
    {
        return;
    }

    for (const String &topic : _haDiscoveryTopics)
    {
        if (!containsTopic(currentTopics, topic))
        {
            _mqtt.publish(topic.c_str(), "", true);
        }
    }

    _haDiscoveryTopics = currentTopics;
}

void MqttHandler::publishHaSection(const char *stateSection,
                                   JsonObjectConst object,
                                   const HaEntityDescriptor *descriptors,
                                   size_t descriptorCount,
                                   std::vector<String> &currentTopics,
                                   bool force)
{
    const String topicBase = baseTopic();
    const String availabilityTopic = topicBase + "/Alive";

    for (JsonPairConst entry : object)
    {
        JsonVariantConst value = entry.value();
        if (!isDiscoverableValue(value))
        {
            continue;
        }

        const char *key = entry.key().c_str();
        const HaEntityDescriptor *descriptor = findDescriptor(key, descriptors, descriptorCount);
        const bool binarySensor = value.is<bool>();
        const char *component = binarySensor ? "binary_sensor" : "sensor";
        const String topic = buildDiscoveryTopic(topicBase, component, key);

        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            continue;
        }

        JsonDocument doc;
        doc["name"] = key;
        doc["stat_t"] = topicBase + "/" + stateSection + "/" + key;
        doc["avty_t"] = availabilityTopic;
        doc["pl_avail"] = "true";
        doc["pl_not_avail"] = "false";
        doc["uniq_id"] = buildUniqueId(_settings.get.mqttTopic(), stateSection, key);
        if (binarySensor)
        {
            doc["pl_on"] = "true";
            doc["pl_off"] = "false";
        }
        if (descriptor != nullptr && descriptor->icon != nullptr && descriptor->icon[0] != '\0')
        {
            doc["ic"] = String("mdi:") + descriptor->icon;
        }
        if (descriptor != nullptr && descriptor->unit != nullptr && descriptor->unit[0] != '\0')
        {
            doc["unit_of_meas"] = descriptor->unit;
        }
        if (descriptor != nullptr && descriptor->deviceClass != nullptr && descriptor->deviceClass[0] != '\0')
        {
            doc["dev_cla"] = descriptor->deviceClass;
        }

        populateDeviceInfo(doc);

        String payload;
        serializeJson(doc, payload);

        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    }
}

void MqttHandler::publishHaDs18b20(JsonObjectConst espData, std::vector<String> &currentTopics, bool force)
{
    const String topicBase = baseTopic();
    const String availabilityTopic = topicBase + "/Alive";

    for (JsonPairConst entry : espData)
    {
        const char *key = entry.key().c_str();
        if (strncmp(key, "DS18B20_", 8) != 0 || !isDiscoverableValue(entry.value()))
        {
            continue;
        }

        const String topic = buildDiscoveryTopic(topicBase, "sensor", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            continue;
        }

        JsonDocument doc;
        doc["name"] = key;
        doc["stat_t"] = topicBase + "/" + key;
        doc["avty_t"] = availabilityTopic;
        doc["pl_avail"] = "true";
        doc["pl_not_avail"] = "false";
        doc["uniq_id"] = buildUniqueId(_settings.get.mqttTopic(), "EspData", key);
        doc["ic"] = "mdi:thermometer-lines";
        doc["unit_of_meas"] = HA_UNIT_CELSIUS;
        doc["dev_cla"] = "temperature";

        populateDeviceInfo(doc);

        String payload;
        serializeJson(doc, payload);

        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    }
}

bool MqttHandler::hasHaDiscoveryTopic(const String &topic) const
{
    return containsTopic(_haDiscoveryTopics, topic);
}

void MqttHandler::setupSubscriptions()
{
    const String commandTopic = baseTopic() + "/DeviceControl/Set_Command";
    _mqtt.subscribe(commandTopic.c_str());

    if (strlen(_settings.get.mqttTriggerPath()) > 0)
    {
        _mqtt.subscribe(_settings.get.mqttTriggerPath());
    }
}

String MqttHandler::baseTopic() const
{
    return String(_settings.get.mqttTopic());
}
