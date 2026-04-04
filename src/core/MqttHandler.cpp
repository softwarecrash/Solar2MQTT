#include "core/MqttHandler.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "core/SettingsPrefs.h"
#include "core/SolarState.h"
#include "core/WiFiManager.h"
#include "solar/HaDiscoveryCatalog.h"
#include "solar/SolarInverterService.h"

extern void writeLog(const char *format, ...);
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

String sanitizeRawMqttText(const char *value)
{
    if (value == nullptr || value[0] == '\0')
    {
        return String();
    }

    String sanitized;
    sanitized.reserve(strlen(value));

    bool lastWasSpace = true;
    for (const char *cursor = value; *cursor != '\0'; ++cursor)
    {
        const unsigned char c = static_cast<unsigned char>(*cursor);
        if (c >= 33 && c <= 126)
        {
            sanitized += static_cast<char>(c);
            lastWasSpace = false;
        }
        else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            if (!lastWasSpace && !sanitized.isEmpty())
            {
                sanitized += ' ';
                lastWasSpace = true;
            }
        }
    }

    sanitized.trim();
    return sanitized;
}

bool isNumericCsvPayload(const String &value)
{
    bool hasComma = false;
    bool hasDigit = false;

    for (size_t i = 0; i < value.length(); ++i)
    {
        const char c = value.charAt(i);
        if (c >= '0' && c <= '9')
        {
            hasDigit = true;
            continue;
        }

        if (c == ',')
        {
            hasComma = true;
            continue;
        }

        if (c == '.' || c == '-' || c == '+' || c == ' ')
        {
            continue;
        }

        return false;
    }

    return hasComma && hasDigit;
}

String normalizeRawPayloadForBroker(const String &value)
{
    if (!isNumericCsvPayload(value))
    {
        return value;
    }

    // Some MQTT server adapters reinterpret pure numeric CSV payloads as character codes.
    // Prefix them so the payload survives the broker unchanged while staying readable.
    return String("raw:") + value;
}

String mqttBytesToHexPreview(const String &value, size_t maxBytes = 64)
{
    static const char hexDigits[] = "0123456789ABCDEF";

    const size_t bytesToShow = (value.length() < maxBytes) ? value.length() : maxBytes;
    String hex;
    hex.reserve(bytesToShow * 3 + 4);

    for (size_t i = 0; i < bytesToShow; ++i)
    {
        const uint8_t byteValue = static_cast<uint8_t>(value.charAt(i));
        if (!hex.isEmpty())
        {
            hex += ' ';
        }
        hex += hexDigits[(byteValue >> 4) & 0x0F];
        hex += hexDigits[byteValue & 0x0F];
    }

    if (value.length() > maxBytes)
    {
        hex += " ...";
    }

    return hex;
}

bool shouldDumpRawMqttKey(const char *key)
{
    return strcmp(key, "QPIGS") == 0 ||
           strcmp(key, "QPIRI") == 0 ||
           strcmp(key, "Q1") == 0 ||
           strcmp(key, "QPIWS") == 0;
}

String getHaDeviceId()
{
    const uint64_t mac = ESP.getEfuseMac();
    char id[32];
    snprintf(id,
             sizeof(id),
             "solar2mqtt_%04X%08X",
             static_cast<uint16_t>(mac >> 32),
             static_cast<uint32_t>(mac));
    return String(id);
}

String buildUniqueId(const String &deviceId, const char *sectionName, const char *key)
{
    return deviceId + "." + sectionName + "." + key;
}

String getDiscoveryModel(JsonDocument &snapshot)
{
    JsonObjectConst deviceData = snapshot["DeviceData"].as<JsonObjectConst>();
    JsonObjectConst status = snapshot["Status"].as<JsonObjectConst>();

    const char *deviceModel = deviceData[DESCR_Device_Model] | nullptr;
    if (deviceModel != nullptr && deviceModel[0] != '\0')
    {
        return String(deviceModel);
    }

    const char *machineType = deviceData[DESCR_Machine_Type] | nullptr;
    if (machineType != nullptr && machineType[0] != '\0')
    {
        return String(machineType);
    }

    const char *protocol = status["protocol"] | nullptr;
    if (protocol != nullptr && protocol[0] != '\0')
    {
        return String(protocol);
    }

    return "Solar Inverter";
}

void populateDeviceInfo(JsonDocument &doc, JsonDocument &snapshot)
{
    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"][0] = getHaDeviceId();
    device["name"] = _settings.get.deviceName();
    device["manufacturer"] = "SoftWareCrash";
    device["model"] = getDiscoveryModel(snapshot);
    device["sw_version"] = STRVERSION;
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
      _pendingLegacyDs18Cleanup(true),
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
    _pendingLegacyDs18Cleanup = true;
    _haDiscoveryTopics.clear();
}

void MqttHandler::reconfigure()
{
    _mqtt.disconnect();
    _plainClient.stop();
    _secureClient.stop();

    configureClient();
    _configured = strlen(_settings.get.mqttHost()) > 0;
    _pendingFullPublish = _configured;
    _pendingHaDiscovery = _configured && _settings.get.mqttHAEnabled();
    _forceHaDiscovery = _pendingHaDiscovery;
    _pendingLegacyDs18Cleanup = true;
    _lastConnected = false;
    _lastReconnectAttempt = 0;
    _lastAlivePublish = millis();
    _lastStatePublish = millis();
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

    if (connected && _pendingFullPublish)
    {
        _pendingFullPublish = false;
        publishState();

        if (_settings.get.mqttHAEnabled() && !_pendingHaDiscovery)
        {
            publishHaDiscovery(false);
        }
    }

    if (connected && _pendingHaDiscovery)
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

    const String topic = baseTopic() + "/LiveData/DS18B20_" + String(static_cast<unsigned>(index));
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", static_cast<double>(temperature));
    _mqtt.publish(topic.c_str(), payload, true);
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

    if (_pendingLegacyDs18Cleanup)
    {
        JsonDocument snapshot;
        _state.snapshotTo(snapshot);
        for (JsonPairConst entry : snapshot["LiveData"].as<JsonObjectConst>())
        {
            const char *key = entry.key().c_str();
            if (strncmp(key, "DS18B20_", 8) != 0)
            {
                continue;
            }

            const String legacyRootTopic = baseTopic() + "/" + key;
            const String legacyEspDataTopic = baseTopic() + "/EspData/" + key;
            _mqtt.publish(legacyRootTopic.c_str(), "", true);
            _mqtt.publish(legacyEspDataTopic.c_str(), "", true);
        }
        _pendingLegacyDs18Cleanup = false;
    }
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
    }
}

void MqttHandler::publishRawState(JsonObjectConst rawObject)
{
    for (JsonPairConst entry : rawObject)
    {
        const char *key = entry.key().c_str();
        const String topic = baseTopic() + "/RAW/" + key;
        JsonVariantConst value = entry.value();
        if (value.isNull())
        {
            _mqtt.publish(topic.c_str(), "", false);
            continue;
        }

        String payload = value.as<String>();
        if (payload.isEmpty())
        {
            publishJsonValue(_mqtt, topic, value, false);
            continue;
        }

        payload = sanitizeRawMqttText(payload.c_str());
        payload = normalizeRawPayloadForBroker(payload);
        if (shouldDumpRawMqttKey(key))
        {
            writeLog("[MQTT][RAW] key=%s len=%u hex=%s txt=\"%s\"",
                     key,
                     static_cast<unsigned>(payload.length()),
                     mqttBytesToHexPreview(payload, 96).c_str(),
                     payload.c_str());
        }
        _mqtt.publish(topic.c_str(), payload.c_str(), false);
    }
}

void MqttHandler::publishHaDiscovery(bool force)
{
    JsonDocument snapshot;
    _state.snapshotTo(snapshot);

    std::vector<String> currentTopics;
    currentTopics.reserve(_haDiscoveryTopics.size() + 16);

    publishHaSection(snapshot,
                     "DeviceData",
                     snapshot["DeviceData"].as<JsonObjectConst>(),
                     HA_STATIC_DESCRIPTORS,
                     sizeof(HA_STATIC_DESCRIPTORS) / sizeof(HaEntityDescriptor),
                     currentTopics,
                     force);
    publishHaSection(snapshot,
                     "LiveData",
                     snapshot["LiveData"].as<JsonObjectConst>(),
                     HA_LIVE_DESCRIPTORS,
                     sizeof(HA_LIVE_DESCRIPTORS) / sizeof(HaEntityDescriptor),
                     currentTopics,
                     force);
    publishHaDs18b20(snapshot, snapshot["LiveData"].as<JsonObjectConst>(), currentTopics, force);

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

void MqttHandler::publishHaSection(JsonDocument &snapshot,
                                   const char *stateSection,
                                   JsonObjectConst object,
                                   const HaEntityDescriptor *descriptors,
                                   size_t descriptorCount,
                                   std::vector<String> &currentTopics,
                                   bool force)
{
    const String topicBase = baseTopic();
    const String deviceId = getHaDeviceId();
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
        const String topic = buildDiscoveryTopic(deviceId, component, key);

        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            continue;
        }

        JsonDocument doc;
        doc["name"] = key;
        doc["state_topic"] = topicBase + "/" + stateSection + "/" + key;
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, stateSection, key);
        doc["force_update"] = true;
        doc["qos"] = 1;
        if (binarySensor)
        {
            doc["payload_on"] = "true";
            doc["payload_off"] = "false";
        }
        if (descriptor != nullptr && descriptor->icon != nullptr && descriptor->icon[0] != '\0')
        {
            doc["icon"] = String("mdi:") + descriptor->icon;
        }
        if (descriptor != nullptr && descriptor->unit != nullptr && descriptor->unit[0] != '\0')
        {
            doc["unit_of_measurement"] = descriptor->unit;
        }
        if (descriptor != nullptr && descriptor->deviceClass != nullptr && descriptor->deviceClass[0] != '\0')
        {
            doc["device_class"] = descriptor->deviceClass;
        }

        populateDeviceInfo(doc, snapshot);

        String payload;
        serializeJson(doc, payload);

        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    }
}

void MqttHandler::publishHaDs18b20(JsonDocument &snapshot, JsonObjectConst liveValues, std::vector<String> &currentTopics, bool force)
{
    const String topicBase = baseTopic();
    const String deviceId = getHaDeviceId();
    const String availabilityTopic = topicBase + "/Alive";

    for (JsonPairConst entry : liveValues)
    {
        const char *key = entry.key().c_str();
        if (strncmp(key, "DS18B20_", 8) != 0 || !isDiscoverableValue(entry.value()))
        {
            continue;
        }

        const String topic = buildDiscoveryTopic(deviceId, "sensor", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            continue;
        }

        JsonDocument doc;
        doc["name"] = key;
        doc["state_topic"] = topicBase + "/LiveData/" + key;
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "EspData", key);
        doc["icon"] = "mdi:thermometer-lines";
        doc["unit_of_measurement"] = HA_UNIT_CELSIUS;
        doc["device_class"] = "temperature";
        doc["force_update"] = true;
        doc["qos"] = 1;

        populateDeviceInfo(doc, snapshot);

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
