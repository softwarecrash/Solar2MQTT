#include "core/MqttHandler.h"

#include <ArduinoJson.h>
#include <initializer_list>
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

bool isPowMrProtocolName(const char *protocol)
{
    return protocol != nullptr &&
           (strcmp(protocol, "MODBUS_POWMR") == 0 ||
            strcmp(protocol, "MODBUS_POWMR_PI") == 0);
}

bool isPowMrPiHybridProtocolName(const char *protocol)
{
    return protocol != nullptr && strcmp(protocol, "MODBUS_POWMR_PI") == 0;
}


bool isPowMrParallelOnlyKey(const char *key)
{
    if (key == nullptr)
    {
        return false;
    }

    const char *const keys[] = {
        DESCR_PV_OK_Condition_For_Parallel,
        DESCR_PV_Power_Balance,
        DESCR_Parallel_Max_Num,
    };

    for (const char *candidate : keys)
    {
        if (strcmp(key, candidate) == 0)
        {
            return true;
        }
    }
    return false;
}

bool isPowMrWritableSettingKey(const char *key)
{
    if (key == nullptr)
    {
        return false;
    }

    const char *const keys[] = {
        DESCR_Charger_Source_Priority,
        DESCR_Output_Source_Priority,
        DESCR_Input_Voltage_Range,
        "Battery_Type",
        DESCR_AC_Out_Rating_Frequency,
        DESCR_Current_Max_Charging_Current,
        DESCR_AC_Out_Rating_Voltage,
        DESCR_Current_Max_AC_Charging_Current,
        DESCR_Battery_Recharge_Voltage,
        DESCR_Battery_Redischarge_Voltage,
        DESCR_Battery_Bulk_Voltage,
        DESCR_Battery_Float_Voltage,
        DESCR_Battery_Under_Voltage,
        "Battery_Equalization_Voltage",
        "Battery_Equalization_Time",
        "Battery_Equalization_Timeout",
        "Battery_Equalization_Interval",
        DESCR_Battery_Equalization_Enabled,
        DESCR_Buzzer_Enabled,
        DESCR_Overload_Bypass_Enabled,
        DESCR_Power_Saving_Enabled,
        DESCR_LCD_Reset_To_Default_Enabled,
        DESCR_Data_Log_Pop_Up,
        DESCR_Overload_Restart_Enabled,
        DESCR_Over_Temperature_Restart_Enabled,
        DESCR_LCD_Backlight_Enabled,
        DESCR_Primary_Source_Interrupt_Alarm_Enabled,
        DESCR_Record_Fault_Code_Enabled,
        DESCR_Solar_Feed_To_Grid_Enabled,
    };

    for (const char *candidate : keys)
    {
        if (strcmp(key, candidate) == 0)
        {
            return true;
        }
    }
    return false;
}

bool isDiscoverableValue(JsonVariantConst value)
{
    return !value.isNull() && !value.is<JsonObjectConst>() && !value.is<JsonArrayConst>();
}

String buildDiscoveryTopic(const String &baseTopic, const char *component, const char *key)
{
    return String("homeassistant/") + component + "/" + baseTopic + "/" + key + "/config";
}

void purgeHaDiscoveryKey(PubSubClient &client, const String &deviceId, const char *key)
{
    const char *const components[] = {"sensor", "binary_sensor", "number", "select", "switch"};
    for (const char *component : components)
    {
        const String topic = buildDiscoveryTopic(deviceId, component, key);
        client.publish(topic.c_str(), "", true);
    }
}

void purgeHaDiscoveryComponent(PubSubClient &client,
                               const String &deviceId,
                               const char *component,
                               const char *key)
{
    const String topic = buildDiscoveryTopic(deviceId, component, key);
    client.publish(topic.c_str(), "", true);
}

bool stringEqualsAny(const char *value, const char *const *items, size_t count)
{
    if (value == nullptr)
    {
        return false;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (strcmp(value, items[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

bool isApprovedHaDiscoveryKey(const char *component, const char *key, bool powMr)
{
    if (component == nullptr || key == nullptr)
    {
        return false;
    }

    if (strcmp(component, "sensor") == 0 || strcmp(component, "binary_sensor") == 0)
    {
        if (strcmp(key, DESCR_ESP_Internal_Temperature) == 0 ||
            strncmp(key, "DS18B20_", 8) == 0)
        {
            return true;
        }

        // PowMr writable settings are exposed as select/number entities.
        // Purge older generic sensor discovery for the same keys.
        if (powMr && (isPowMrWritableSettingKey(key) ||
                      isPowMrParallelOnlyKey(key)))
        {
            return false;
        }

        return findDescriptor(key,
                              HA_STATIC_DESCRIPTORS,
                              sizeof(HA_STATIC_DESCRIPTORS) / sizeof(HaEntityDescriptor)) != nullptr ||
               findDescriptor(key,
                              HA_LIVE_DESCRIPTORS,
                              sizeof(HA_LIVE_DESCRIPTORS) / sizeof(HaEntityDescriptor)) != nullptr;
    }

    if (!powMr)
    {
        return false;
    }

    if (strcmp(component, "select") == 0)
    {
        const char *const selectKeys[] = {
            DESCR_Output_Source_Priority,
            DESCR_Charger_Source_Priority,
            DESCR_Input_Voltage_Range,
            "Battery_Type",
            DESCR_AC_Out_Rating_Frequency,
        };
        return stringEqualsAny(key, selectKeys, sizeof(selectKeys) / sizeof(selectKeys[0]));
    }

    if (strcmp(component, "switch") == 0)
    {
        const char *const switchKeys[] = {
            // Keep Home Assistant focused on settings that are useful to
            // automate. Panel/service-only flags live in the Web UI.
            DESCR_Battery_Equalization_Enabled,
            DESCR_Overload_Bypass_Enabled,
            DESCR_Power_Saving_Enabled,
            DESCR_Overload_Restart_Enabled,
            DESCR_Over_Temperature_Restart_Enabled,
            DESCR_Solar_Feed_To_Grid_Enabled,
        };
        return stringEqualsAny(key, switchKeys, sizeof(switchKeys) / sizeof(switchKeys[0]));
    }

    if (strcmp(component, "number") == 0)
    {
        const char *const numberKeys[] = {
            DESCR_Current_Max_Charging_Current,
            DESCR_AC_Out_Rating_Voltage,
            DESCR_Current_Max_AC_Charging_Current,
            DESCR_Battery_Recharge_Voltage,
            DESCR_Battery_Redischarge_Voltage,
            DESCR_Battery_Bulk_Voltage,
            DESCR_Battery_Float_Voltage,
            DESCR_Battery_Under_Voltage,
            "Battery_Equalization_Voltage",
            "Battery_Equalization_Time",
            "Battery_Equalization_Timeout",
            "Battery_Equalization_Interval",
        };
        return stringEqualsAny(key, numberKeys, sizeof(numberKeys) / sizeof(numberKeys[0]));
    }

    return false;
}

bool parseOwnHaDiscoveryTopic(const String &topic,
                              const String &deviceId,
                              String &component,
                              String &key)
{
    const String prefix = "homeassistant/";
    if (!topic.startsWith(prefix) || !topic.endsWith("/config"))
    {
        return false;
    }

    const int componentEnd = topic.indexOf('/', prefix.length());
    if (componentEnd < 0)
    {
        return false;
    }

    component = topic.substring(prefix.length(), componentEnd);

    const String deviceMarker = "/" + deviceId + "/";
    if (!topic.substring(componentEnd).startsWith(deviceMarker))
    {
        return false;
    }

    const int keyStart = componentEnd + deviceMarker.length();
    const int keyEnd = topic.length() - 7; // strlen("/config")
    if (keyEnd <= keyStart)
    {
        return false;
    }

    key = topic.substring(keyStart, keyEnd);
    return key.length() > 0;
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

void populateEqualizationDeviceInfo(JsonDocument &doc, JsonDocument &snapshot)
{
    const String parentDeviceId = getHaDeviceId();
    const String equalizationDeviceId = parentDeviceId + "_equalization";

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"][0] = equalizationDeviceId;
    device["name"] = String(_settings.get.deviceName()) + " — Выравнивание АКБ";
    device["manufacturer"] = "SoftWareCrash";
    device["model"] = "PowMr battery equalization";
    device["sw_version"] = STRVERSION;
    device["via_device"] = parentDeviceId;
}

bool isPowMrEqualizationKey(const char *key)
{
    if (key == nullptr)
    {
        return false;
    }

    const char *const keys[] = {
        DESCR_Battery_Equalization_Enabled,
        DESCR_Battery_Equalization_Active,
        "Battery_Equalization_Voltage",
        "Battery_Equalization_Time",
        "Battery_Equalization_Timeout",
        "Battery_Equalization_Interval",
    };
    return stringEqualsAny(key, keys, sizeof(keys) / sizeof(keys[0]));
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
      _energyBacklog(),
      _pendingFullPublish(false),
      _pendingHaDiscovery(false),
      _forceHaDiscovery(false),
      _pendingLegacyDs18Cleanup(true),
      _configured(false),
      _lastConnected(false),
      _replayingEnergyBacklog(false),
      _haDiscoverySweepActive(false),
      _haDiscoverySweepPowMr(false),
      _haDiscoverySweepStartedMs(0),
      _haDiscoverySweepTopic(),
      _pendingDelayedHaDiscovery(false),
      _delayedHaDiscoveryAt(0),
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
    _pendingFullPublish = false;
    _lastReconnectAttempt = 0;
    _lastAlivePublish = millis();
    _lastStatePublish = millis();
    _lastConnected = false;
    _forceHaDiscovery = false;
    _pendingLegacyDs18Cleanup = true;
    _haDiscoveryTopics.clear();
    _replayingEnergyBacklog = false;
    _haDiscoverySweepActive = false;
    _haDiscoverySweepPowMr = false;
    _haDiscoverySweepStartedMs = 0;
    _haDiscoverySweepTopic = "";
    _pendingDelayedHaDiscovery = false;
    _delayedHaDiscoveryAt = 0;
    _energyBacklog.begin();
}

void MqttHandler::reconfigure()
{
    _mqtt.disconnect();
    _plainClient.stop();
    _secureClient.stop();

    configureClient();
    _configured = strlen(_settings.get.mqttHost()) > 0;
    _pendingFullPublish = _configured && usesImmediateStatePublishing();
    _pendingHaDiscovery = _configured && _settings.get.mqttHAEnabled();
    _forceHaDiscovery = _pendingHaDiscovery;
    _pendingLegacyDs18Cleanup = true;
    _lastConnected = false;
    _lastReconnectAttempt = 0;
    _lastAlivePublish = millis();
    _lastStatePublish = millis();
    _haDiscoveryTopics.clear();
    _replayingEnergyBacklog = false;
    _haDiscoverySweepActive = false;
    _haDiscoverySweepPowMr = false;
    _haDiscoverySweepStartedMs = 0;
    _haDiscoverySweepTopic = "";
    _pendingDelayedHaDiscovery = false;
    _delayedHaDiscoveryAt = 0;
    _energyBacklog.cancelReplay();
}

void MqttHandler::loop()
{
    const unsigned long now = millis();
    bool connected = false;

    if (_configured && _wifiManager.getConnectionState())
    {
        connected = ensureConnected();
        if (connected)
        {
            _mqtt.loop();
            if (_haDiscoverySweepActive &&
                (now - _haDiscoverySweepStartedMs) >= 5000UL)
            {
                stopHaDiscoverySweep();
            }

            if (_pendingDelayedHaDiscovery &&
                static_cast<int32_t>(now - _delayedHaDiscoveryAt) >= 0)
            {
                _pendingDelayedHaDiscovery = false;
                if (_settings.get.mqttHAEnabled())
                {
                    _pendingHaDiscovery = true;
                    _forceHaDiscovery = true;
                    writeLog("[HA] Delayed forced discovery refresh");
                }
            }
        }
    }

    // Battery charge/discharge totals are accumulated locally from the
    // freshest inverter voltage/current readings. This runs independently
    // from MQTT so an outage never creates an energy gap.
    _energyBacklog.updateBatteryEnergy(_state, _inverterService.isConnected(), now);

    // Keep the cumulative inverter-reported PV/grid energy counters in
    // LittleFS while MQTT is unavailable.
    _energyBacklog.captureIfNeeded(_state, _configured && !connected, now);

    if (!_configured)
    {
        _replayingEnergyBacklog = false;
        _energyBacklog.cancelReplay();
        _lastConnected = false;
        return;
    }

    if (!connected)
    {
        if (_lastConnected || _replayingEnergyBacklog)
        {
            _energyBacklog.cancelReplay();
        }
        _replayingEnergyBacklog = false;
        _lastConnected = false;
        return;
    }

    if (!_lastConnected)
    {
        _replayingEnergyBacklog = _energyBacklog.startReplay();
    }

    if (_replayingEnergyBacklog)
    {
        const EnergyBacklog::ReplayResult replay =
            _energyBacklog.replayBatch(_mqtt, baseTopic(), 2);

        if (replay == EnergyBacklog::ReplayResult::InProgress)
        {
            _lastConnected = true;
            return;
        }

        _replayingEnergyBacklog = false;

        if (replay == EnergyBacklog::ReplayResult::Complete)
        {
            // Historical counter states have been replayed. Publish the live
            // snapshot immediately afterwards so retained topics finish on
            // the current values.
            _pendingFullPublish = true;
        }
        else if (replay == EnergyBacklog::ReplayResult::Failed)
        {
            // Keep the file intact and force a normal MQTT reconnect before
            // trying the backlog again.
            writeLog("[EnergyBacklog] Replay interrupted; MQTT reconnect scheduled");
            _mqtt.disconnect();
            _lastConnected = false;
            return;
        }
    }

    if ((now - _lastAlivePublish) >= 30000UL)
    {
        _lastAlivePublish = now;
        publishAlive();
    }

    const uint32_t intervalMs = statePublishIntervalMs();
    if (intervalMs > 0 && (now - _lastStatePublish) >= intervalMs)
    {
        _pendingFullPublish = true;
    }

    if (_pendingFullPublish)
    {
        _pendingFullPublish = false;
        publishState();
        _lastStatePublish = now;

        if (_settings.get.mqttHAEnabled() && !_pendingHaDiscovery)
        {
            publishHaDiscovery(false);
        }
    }

    if (_pendingHaDiscovery)
    {
        const bool force = _forceHaDiscovery;
        _pendingHaDiscovery = false;
        _forceHaDiscovery = false;
        publishHaDiscovery(force);
    }

    _lastConnected = true;
}

bool MqttHandler::isConnected()
{
    return _mqtt.connected();
}

void MqttHandler::triggerFullStatePublish()
{
    if (usesImmediateStatePublishing())
    {
        _pendingFullPublish = true;
    }
}

void MqttHandler::triggerHaDiscovery()
{
    _pendingHaDiscovery = true;
    _forceHaDiscovery = true;
}

void MqttHandler::publishSensorImmediate(uint8_t index, float temperature)
{
    if (!_mqtt.connected() || !usesImmediateStatePublishing())
    {
        return;
    }

    const String topic = baseTopic() + "/LiveData/DS18B20_" + String(static_cast<unsigned>(index));
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", static_cast<double>(temperature));
    _mqtt.publish(topic.c_str(), payload, true);
}

void MqttHandler::flushPersistentEnergy()
{
    _energyBacklog.flushBatteryEnergy();
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

    if (_haDiscoverySweepActive)
    {
        String component;
        String key;
        if (parseOwnHaDiscoveryTopic(topicString, getHaDeviceId(), component, key))
        {
            if (length > 0 &&
                !isApprovedHaDiscoveryKey(component.c_str(), key.c_str(), _haDiscoverySweepPowMr))
            {
                _mqtt.publish(topicString.c_str(), "", true);
                writeLog("[HA] Removed stale discovery: %s", topicString.c_str());
            }
            return;
        }
    }

    if (strlen(_settings.get.mqttTriggerPath()) > 0 && topicString == _settings.get.mqttTriggerPath())
    {
        triggerFullStatePublish();
        return;
    }

    const String commandTopic = baseTopic() + "/DeviceControl/Set_Command";
    if (topicString == commandTopic)
    {
        _inverterService.queueCommand(message);
        triggerFullStatePublish();
    }
}

uint32_t MqttHandler::statePublishIntervalMs() const
{
    return static_cast<uint32_t>(_settings.get.mqttRefresh()) * 1000UL;
}

bool MqttHandler::usesImmediateStatePublishing() const
{
    return statePublishIntervalMs() == 0;
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
    startHaDiscoverySweep();

    // Explicit migration cleanup for legacy PowMr diagnostic entities and
    // generic sensor duplicates that were created before settings received
    // dedicated select/number discovery entities.
    {
        const String deviceId = getHaDeviceId();

        const char *const obsoleteDebugKeys[] = {
            "PowMr_Debug_4556",
            "PowMr_Debug_4558",
            "PowMr_Debug_4559",
            "PowMr_Debug_4560",
            "PowMr_Debug_4561",
            "PowMr_Status_Flags_1",
            "PowMr_Status_Flags_2",
            "PowMr_Settings_Flags",
        };
        for (const char *key : obsoleteDebugKeys)
        {
            purgeHaDiscoveryKey(_mqtt, deviceId, key);
        }

        const char *const oldSettingSensorKeys[] = {
            DESCR_Charger_Source_Priority,
            DESCR_Output_Source_Priority,
            DESCR_Input_Voltage_Range,
            "Battery_Type",
            DESCR_AC_Out_Rating_Frequency,
            DESCR_Current_Max_Charging_Current,
            DESCR_AC_Out_Rating_Voltage,
            DESCR_Current_Max_AC_Charging_Current,
            DESCR_Battery_Recharge_Voltage,
            DESCR_Battery_Redischarge_Voltage,
            DESCR_Battery_Bulk_Voltage,
            DESCR_Battery_Float_Voltage,
            DESCR_Battery_Under_Voltage,
            "Battery_Equalization_Voltage",
            "Battery_Equalization_Time",
            "Battery_Equalization_Timeout",
            "Battery_Equalization_Interval",
            DESCR_Buzzer_Enabled,
            DESCR_Overload_Bypass_Enabled,
            DESCR_Power_Saving_Enabled,
            DESCR_LCD_Reset_To_Default_Enabled,
            DESCR_Data_Log_Pop_Up,
            DESCR_Overload_Restart_Enabled,
            DESCR_Over_Temperature_Restart_Enabled,
            DESCR_LCD_Backlight_Enabled,
            DESCR_Primary_Source_Interrupt_Alarm_Enabled,
            DESCR_Record_Fault_Code_Enabled,
            DESCR_Solar_Feed_To_Grid_Enabled,
        };
        for (const char *key : oldSettingSensorKeys)
        {
            purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
            purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);
        }
    }

    // Run one more forced discovery after startup/static polling settles so
    // retained DeviceData configs are guaranteed to receive the Russian names.
    _pendingDelayedHaDiscovery = _settings.get.mqttHAEnabled();
    _delayedHaDiscoveryAt = millis() + 10000UL;

    // Remove stale Home Assistant entities created by an earlier, incorrect
    // assumption that PowMr/Victor lithium menu 12/13 values were SOC.
    // The retained discovery/state topics can otherwise survive firmware
    // changes and display nonsense such as 510%/540%.
    {
        const char *const obsoletePowMrSocKeys[] = {
            DESCR_Battery_Back_To_Utility_SOC,
            DESCR_Battery_Back_To_Battery_SOC,
        };
        const char *const components[] = {"sensor", "number", "select"};
        const String deviceId = getHaDeviceId();

        for (const char *key : obsoletePowMrSocKeys)
        {
            for (const char *component : components)
            {
                const String discoveryTopic = buildDiscoveryTopic(deviceId, component, key);
                _mqtt.publish(discoveryTopic.c_str(), "", true);
            }

            const String stateTopic = baseTopic() + "/DeviceData/" + key;
            _mqtt.publish(stateTopic.c_str(), "", true);
        }
    }

    // Purge retained Home Assistant Discovery for values that are present in
    // runtime JSON but are not part of the supported HA catalog. Older builds
    // auto-discovered every scalar field, so diagnostic/probe values could
    // survive indefinitely as stale entities in Home Assistant.
    {
        JsonDocument snapshot;
        _state.snapshotTo(snapshot);
        const String deviceId = getHaDeviceId();

        for (JsonPairConst entry : snapshot["DeviceData"].as<JsonObjectConst>())
        {
            const char *key = entry.key().c_str();
            if (isPowMrWritableSettingKey(key))
            {
                continue;
            }
            if (findDescriptor(key,
                               HA_STATIC_DESCRIPTORS,
                               sizeof(HA_STATIC_DESCRIPTORS) / sizeof(HaEntityDescriptor)) == nullptr)
            {
                purgeHaDiscoveryKey(_mqtt, deviceId, key);
            }
        }

        for (JsonPairConst entry : snapshot["LiveData"].as<JsonObjectConst>())
        {
            const char *key = entry.key().c_str();
            if (strncmp(key, "DS18B20_", 8) == 0)
            {
                continue;
            }
            if (findDescriptor(key,
                               HA_LIVE_DESCRIPTORS,
                               sizeof(HA_LIVE_DESCRIPTORS) / sizeof(HaEntityDescriptor)) == nullptr)
            {
                purgeHaDiscoveryKey(_mqtt, deviceId, key);
            }
        }
    }

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
    _pendingFullPublish = usesImmediateStatePublishing();
    if (!usesImmediateStatePublishing())
    {
        _lastStatePublish = millis();
    }
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
        _mqtt.publish(topic.c_str(), payload.c_str(), false);
    }
}

void MqttHandler::publishHaDiscovery(bool force)
{
    JsonDocument snapshot;
    _state.snapshotTo(snapshot);

    if (force)
    {
        const char *protocol = snapshot["Status"]["protocol"] | "";
        if (strcmp(protocol, "MODBUS_POWMR") == 0)
        {
            // Older PI30/QPIRI/QPIGS discovery entries can survive as
            // retained Home Assistant entities after the inverter switches
            // to the native PowMr Modbus protocol. They are not refreshed by
            // MODBUS_POWMR and can show stale or conflicting values.
            const char *const obsoletePowMrPiKeys[] = {
                DESCR_AC_In_Rating_Current,
                DESCR_AC_In_Rating_Voltage,
                DESCR_AC_Out_Percent,
                DESCR_AC_Out_Rating_Active_Power,
                DESCR_AC_Out_Rating_Apparent_Power,
                DESCR_AC_Out_Rating_Current,
                DESCR_Battery_Load,
                DESCR_Battery_Rating_Voltage,
                DESCR_Battery_SCC_Volt,
                DESCR_Battery_Voltage_Offset_Fans_On,
                DESCR_Buzzer_Enabled,
                DESCR_Data_Log_Pop_Up,
                DESCR_Device_Status,
                DESCR_EEPROM_Version,
                DESCR_Inverter_Bus_Temperature,
                DESCR_Inverter_Bus_Voltage,
                DESCR_LCD_Backlight_Enabled,
                DESCR_LCD_Reset_To_Default_Enabled,
                DESCR_Machine_Type,
                DESCR_Max_Charging_Time_At_CV_Stage,
                DESCR_Max_Discharging_Current,
                DESCR_Operation_Logic,
                DESCR_Output_Mode,
                DESCR_Over_Temperature_Restart_Enabled,
                DESCR_Overload_Bypass_Enabled,
                DESCR_Overload_Restart_Enabled,
                DESCR_Parallel_Max_Num,
                DESCR_Power_Saving_Enabled,
                DESCR_Primary_Source_Interrupt_Alarm_Enabled,
                DESCR_PV_Charging_Power,
                DESCR_PV_OK_Condition_For_Parallel,
                DESCR_PV_Power_Balance,
                DESCR_PV1_Input_Current,
                DESCR_Record_Fault_Code_Enabled,
                DESCR_Solar_Feed_To_Grid_Enabled,
                DESCR_Status_Flag,
                DESCR_Topology,
            };

            const String deviceId = getHaDeviceId();
            for (const char *key : obsoletePowMrPiKeys)
            {
                purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
                purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);
            }
        }
        else if (isPowMrPiHybridProtocolName(protocol))
        {
            // In hybrid mode keep the PI-only values we actively refresh, but
            // remove stale duplicates/raw fields that are intentionally not
            // part of the hybrid HA surface.
            const char *const obsoleteHybridKeys[] = {
                DESCR_AC_Out_Percent,
                DESCR_Battery_Load,
                DESCR_Status_Flag,
                DESCR_Battery_Voltage_Offset_Fans_On,
            };
            const String deviceId = getHaDeviceId();
            for (const char *key : obsoleteHybridKeys)
            {
                purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
                purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);
            }
        }
    }

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
    publishHaEspInternalTemperature(snapshot, snapshot["EspData"].as<JsonObjectConst>(), currentTopics, force);
    publishHaDs18b20(snapshot, snapshot["LiveData"].as<JsonObjectConst>(), currentTopics, force);
    publishHaPowMrSettings(snapshot, snapshot["DeviceData"].as<JsonObjectConst>(), currentTopics, force);

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
        const char *activeProtocol = snapshot["Status"]["protocol"] | "";
        if (strcmp(stateSection, "DeviceData") == 0 &&
            isPowMrProtocolName(activeProtocol) &&
            isPowMrWritableSettingKey(key))
        {
            continue;
        }
        if (strcmp(stateSection, "DeviceData") == 0 &&
            isPowMrPiHybridProtocolName(activeProtocol) &&
            isPowMrParallelOnlyKey(key))
        {
            if (force)
            {
                purgeHaDiscoveryKey(_mqtt, deviceId, key);
            }
            continue;
        }

        // Some PowMr/Victor HVM units return Tracker_Temperature=0 from the
        // PI30 Q1 supplement because that sensor is not implemented. Do not
        // expose a misleading 0 °C entity in Home Assistant. On a forced
        // discovery pass also clear any retained discovery config left by an
        // older firmware so Home Assistant removes the stale entity.
        if (strcmp(stateSection, "LiveData") == 0 &&
            isPowMrPiHybridProtocolName(activeProtocol) &&
            strcmp(key, DESCR_Tracker_Temperature) == 0 &&
            value.as<double>() == 0.0)
        {
            if (force)
            {
                const String staleTopic = buildDiscoveryTopic(deviceId, "sensor", key);
                _mqtt.publish(staleTopic.c_str(), "", true);
            }
            continue;
        }

        const HaEntityDescriptor *descriptor = findDescriptor(key, descriptors, descriptorCount);
        if (descriptor == nullptr)
        {
            continue;
        }

        const bool binarySensor = value.is<bool>();
        const char *component = binarySensor ? "binary_sensor" : "sensor";
        const String topic = buildDiscoveryTopic(deviceId, component, key);

        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            continue;
        }

        JsonDocument doc;
        const char *ruName = haRussianName(key);
        doc["name"] = (ruName != nullptr && ruName[0] != '\0')
                          ? ruName
                          : ((descriptor->displayName != nullptr && descriptor->displayName[0] != '\0')
                                 ? descriptor->displayName
                                 : key);
        if (descriptor->defaultEntityId != nullptr && descriptor->defaultEntityId[0] != '\0')
        {
            doc["default_entity_id"] = descriptor->defaultEntityId;
        }
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
        if (descriptor != nullptr && descriptor->stateClass != nullptr && descriptor->stateClass[0] != '\0')
        {
            doc["state_class"] = descriptor->stateClass;
        }

        if (isPowMrProtocolName(activeProtocol) && isPowMrEqualizationKey(key))
        {
            populateEqualizationDeviceInfo(doc, snapshot);
        }
        else
        {
            populateDeviceInfo(doc, snapshot);
        }

        String payload;
        serializeJson(doc, payload);

        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    }
}

void MqttHandler::publishHaEspInternalTemperature(JsonDocument &snapshot,
                                                  JsonObjectConst espValues,
                                                  std::vector<String> &currentTopics,
                                                  bool force)
{
    JsonVariantConst value = espValues[DESCR_ESP_Internal_Temperature];
    if (!isDiscoverableValue(value))
    {
        return;
    }

    const String topicBase = baseTopic();
    const String deviceId = getHaDeviceId();
    const String availabilityTopic = topicBase + "/Alive";
    const String topic = buildDiscoveryTopic(deviceId, "sensor", DESCR_ESP_Internal_Temperature);

    appendTopicIfMissing(currentTopics, topic);
    if (!force && hasHaDiscoveryTopic(topic))
    {
        return;
    }

    JsonDocument doc;
    doc["name"] = "Температура ESP32";
    doc["state_topic"] = topicBase + "/EspData/" + DESCR_ESP_Internal_Temperature;
    doc["availability_topic"] = availabilityTopic;
    doc["payload_available"] = "true";
    doc["payload_not_available"] = "false";
    doc["unique_id"] = buildUniqueId(deviceId, "EspData", DESCR_ESP_Internal_Temperature);
    doc["icon"] = "mdi:thermometer-lines";
    doc["unit_of_measurement"] = HA_UNIT_CELSIUS;
    doc["device_class"] = "temperature";
    doc["state_class"] = "measurement";
    doc["force_update"] = true;
    doc["qos"] = 1;

    populateDeviceInfo(doc, snapshot);

    String payload;
    serializeJson(doc, payload);

    _mqtt.publish(topic.c_str(), payload.c_str(), true);
    appendTopicIfMissing(_haDiscoveryTopics, topic);
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
        doc["name"] = String("Температура ") + key;
        doc["state_topic"] = topicBase + "/LiveData/" + key;
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "EspData", key);
        doc["icon"] = "mdi:thermometer-lines";
        doc["unit_of_measurement"] = HA_UNIT_CELSIUS;
        doc["device_class"] = "temperature";
        doc["state_class"] = "measurement";
        doc["force_update"] = true;
        doc["qos"] = 1;

        populateDeviceInfo(doc, snapshot);

        String payload;
        serializeJson(doc, payload);

        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    }
}

void MqttHandler::publishHaPowMrSettings(JsonDocument &snapshot,
                                          JsonObjectConst deviceValues,
                                          std::vector<String> &currentTopics,
                                          bool force)
{
    JsonObjectConst status = snapshot["Status"].as<JsonObjectConst>();
    const char *protocol = status["protocol"] | "";
    if (!isPowMrProtocolName(protocol))
    {
        return;
    }
    const bool hybridPi = isPowMrPiHybridProtocolName(protocol);

    const String topicBase = baseTopic();
    const String deviceId = getHaDeviceId();
    const String availabilityTopic = topicBase + "/Alive";
    const String commandTopic = topicBase + "/DeviceControl/Set_Command";

    auto publishSelect = [&](const char *key,
                             const char *name,
                             std::initializer_list<const char *> options,
                             const char *commandTemplate)
    {
        JsonVariantConst state = deviceValues[key];
        if (!isDiscoverableValue(state))
        {
            return;
        }

        const String topic = buildDiscoveryTopic(deviceId, "select", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            return;
        }

        JsonDocument doc;
        doc["name"] = name;
        doc["state_topic"] = topicBase + "/DeviceData/" + key;
        doc["command_topic"] = commandTopic;
        doc["command_template"] = commandTemplate;
        if (strcmp(key, DESCR_Output_Source_Priority) == 0)
        {
            // Keep MQTT state payloads stable for existing consumers.
            doc["value_template"] = "{% set modes = {'Utility first': 'UTI — сначала сеть', 'Solar first': 'SUB — солнце → сеть → батарея', 'SBU priority': 'SBU — солнце → батарея → сеть'} %}{{ modes.get(value, value) }}";
        }
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "PowMrSetting", key);
        doc["icon"] = "mdi:tune-variant";
        doc["entity_category"] = "config";
        doc["qos"] = 1;

        JsonArray opts = doc["options"].to<JsonArray>();
        for (const char *option : options)
        {
            opts.add(option);
        }

        populateDeviceInfo(doc, snapshot);

        String payload;
        serializeJson(doc, payload);
        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    };

    auto publishNumber = [&](const char *key,
                             const char *name,
                             const char *settingName,
                             float minValue,
                             float maxValue,
                             float step,
                             const char *unit)
    {
        JsonVariantConst state = deviceValues[key];
        if (!isDiscoverableValue(state))
        {
            return;
        }

        const String topic = buildDiscoveryTopic(deviceId, "number", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            return;
        }

        JsonDocument doc;
        doc["name"] = name;
        doc["state_topic"] = topicBase + "/DeviceData/" + key;
        doc["command_topic"] = commandTopic;
        doc["command_template"] = String("powmr setting ") + settingName + " {{ value }}";
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "PowMrSetting", key);
        doc["icon"] = "mdi:tune";
        doc["min"] = minValue;
        doc["max"] = maxValue;
        doc["step"] = step;
        doc["mode"] = "box";
        doc["entity_category"] = "config";
        doc["qos"] = 1;
        if (unit != nullptr && unit[0] != '\0')
        {
            doc["unit_of_measurement"] = unit;
        }

        if (isPowMrEqualizationKey(key))
        {
            populateEqualizationDeviceInfo(doc, snapshot);
        }
        else
        {
            populateDeviceInfo(doc, snapshot);
        }

        String payload;
        serializeJson(doc, payload);
        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    };

    auto publishSwitch = [&](const char *key,
                              const char *name,
                              char piFlag)
    {
        JsonVariantConst state = deviceValues[key];
        if (!state.is<bool>())
        {
            return;
        }

        const String topic = buildDiscoveryTopic(deviceId, "switch", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            return;
        }

        // Remove the old passive forms if a previous firmware exposed the
        // same QFLAG value as a sensor/binary_sensor.
        purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
        purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);

        JsonDocument doc;
        doc["name"] = name;
        doc["state_topic"] = topicBase + "/DeviceData/" + key;
        doc["command_topic"] = commandTopic;
        doc["payload_on"] = String("powmr pi PE") + piFlag;
        doc["payload_off"] = String("powmr pi PD") + piFlag;
        doc["state_on"] = "true";
        doc["state_off"] = "false";
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "PowMrPiSetting", key);
        doc["icon"] = "mdi:toggle-switch";
        doc["entity_category"] = "config";
        doc["qos"] = 1;

        populateDeviceInfo(doc, snapshot);

        String payload;
        serializeJson(doc, payload);
        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    };

    auto publishCommandSwitch = [&](const char *key,
                                     const char *name,
                                     const char *payloadOn,
                                     const char *payloadOff)
    {
        JsonVariantConst state = deviceValues[key];
        if (!state.is<bool>())
        {
            return;
        }

        const String topic = buildDiscoveryTopic(deviceId, "switch", key);
        appendTopicIfMissing(currentTopics, topic);
        if (!force && hasHaDiscoveryTopic(topic))
        {
            return;
        }

        purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
        purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);

        JsonDocument doc;
        doc["name"] = name;
        doc["state_topic"] = topicBase + "/DeviceData/" + key;
        doc["command_topic"] = commandTopic;
        doc["payload_on"] = payloadOn;
        doc["payload_off"] = payloadOff;
        doc["state_on"] = "true";
        doc["state_off"] = "false";
        doc["availability_topic"] = availabilityTopic;
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unique_id"] = buildUniqueId(deviceId, "PowMrPiSetting", key);
        doc["icon"] = "mdi:battery-sync-outline";
        doc["entity_category"] = "config";
        doc["qos"] = 1;

        if (isPowMrEqualizationKey(key))
        {
            populateEqualizationDeviceInfo(doc, snapshot);
        }
        else
        {
            populateDeviceInfo(doc, snapshot);
        }

        String payload;
        serializeJson(doc, payload);
        _mqtt.publish(topic.c_str(), payload.c_str(), true);
        appendTopicIfMissing(_haDiscoveryTopics, topic);
    };

    publishSelect(DESCR_Output_Source_Priority,
                  "Режим питания нагрузки",
                  {"UTI — сначала сеть", "SUB — солнце → сеть → батарея", "SBU — солнце → батарея → сеть"},
                  "{% if value == 'UTI — сначала сеть' %}powmr outputmode UTI{% elif value == 'SUB — солнце → сеть → батарея' %}powmr outputmode SUB{% elif value == 'SBU — солнце → батарея → сеть' %}powmr outputmode SBU{% endif %}");

    publishSelect(DESCR_Charger_Source_Priority,
                  "Приоритет источника зарядки",
                  {"Utility first", "Solar first", "Solar and Utility", "Solar only"},
                  "{% if value == 'Utility first' %}powmr setting chargerpriority UTILITY{% elif value == 'Solar first' %}powmr setting chargerpriority SOLAR{% elif value == 'Solar and Utility' %}powmr setting chargerpriority SOLAR_UTILITY{% else %}powmr setting chargerpriority SOLAR_ONLY{% endif %}");

    publishSelect(DESCR_Input_Voltage_Range,
                  "Диапазон входного напряжения AC",
                  {"Appliances", "UPS"},
                  "{% if value == 'UPS' %}powmr setting inputrange UPS{% else %}powmr setting inputrange APL{% endif %}");

    publishSelect("Battery_Type",
                  "Тип АКБ / протокол BMS",
                  {"AGM", "FLD", "USE", "LIB", "LIC", "LIP", "LIL"},
                  "powmr batterytype {{ value }}");

    publishSelect(DESCR_AC_Out_Rating_Frequency,
                  "Частота выхода AC",
                  {"50", "60"},
                  "powmr setting outputfreq {{ value }}");

    publishNumber(DESCR_Current_Max_Charging_Current, "Максимальный ток зарядки АКБ", "maxcharge", 0, 120, 1, "A");
    publishNumber(DESCR_AC_Out_Rating_Voltage, "Напряжение выхода AC", "outputvoltage", 220, 240, 10, "V");
    publishNumber(DESCR_Current_Max_AC_Charging_Current, "Максимальный ток зарядки от сети", "utilitycharge", 0, 120, 1, "A");
    publishNumber(DESCR_Battery_Recharge_Voltage, "Напряжение перехода на заряд АКБ", "recharge", 40.0f, 60.0f, 0.1f, "V");
    publishNumber(DESCR_Battery_Redischarge_Voltage, "Напряжение возврата на АКБ", "redischarge", 40.0f, 60.0f, 0.1f, "V");
    publishNumber(DESCR_Battery_Bulk_Voltage, "Напряжение основного заряда АКБ", "bulk", 40.0f, 60.0f, 0.1f, "V");
    publishNumber(DESCR_Battery_Float_Voltage, "Напряжение поддерживающего заряда АКБ", "float", 40.0f, 60.0f, 0.1f, "V");
    publishNumber(DESCR_Battery_Under_Voltage, "Напряжение отключения АКБ", "cutoff", 40.0f, 60.0f, 0.1f, "V");
    publishNumber("Battery_Equalization_Voltage", "Напряжение выравнивания АКБ", "equalizationvoltage", 40.0f, 60.0f, 0.1f, "V");
    publishNumber("Battery_Equalization_Time", "Время выравнивания АКБ", "equalizationtime", 0, 999, 1, "min");
    publishNumber("Battery_Equalization_Timeout", "Тайм-аут выравнивания АКБ", "equalizationtimeout", 0, 999, 1, "min");
    publishNumber("Battery_Equalization_Interval", "Интервал выравнивания АКБ", "equalizationinterval", 0, 999, 1, "d");

    const char *const piSwitchKeys[] = {
        DESCR_Battery_Equalization_Enabled,
        DESCR_Buzzer_Enabled,
        DESCR_Overload_Bypass_Enabled,
        DESCR_Power_Saving_Enabled,
        DESCR_LCD_Reset_To_Default_Enabled,
        DESCR_Data_Log_Pop_Up,
        DESCR_Overload_Restart_Enabled,
        DESCR_Over_Temperature_Restart_Enabled,
        DESCR_LCD_Backlight_Enabled,
        DESCR_Primary_Source_Interrupt_Alarm_Enabled,
        DESCR_Record_Fault_Code_Enabled,
        DESCR_Solar_Feed_To_Grid_Enabled,
    };

    if (hybridPi)
    {
        publishCommandSwitch(DESCR_Battery_Equalization_Enabled,
                             "Выравнивание АКБ",
                             "powmr pi PBEQE1",
                             "powmr pi PBEQE0");
        publishSwitch(DESCR_Overload_Bypass_Enabled, "Байпас при перегрузке", 'b');
        publishSwitch(DESCR_Power_Saving_Enabled, "Режим энергосбережения", 'j');
        publishSwitch(DESCR_Solar_Feed_To_Grid_Enabled, "Разрешение отдачи в сеть", 'd');
        publishSwitch(DESCR_Overload_Restart_Enabled, "Перезапуск после перегрузки", 'u');
        publishSwitch(DESCR_Over_Temperature_Restart_Enabled, "Перезапуск после перегрева", 'v');

        // These are local panel/service preferences. Keep them configurable in
        // the inverter Web UI, but remove them from HA to avoid config clutter.
        const char *const webOnlySwitchKeys[] = {
            DESCR_Buzzer_Enabled,
            DESCR_LCD_Reset_To_Default_Enabled,
            DESCR_Data_Log_Pop_Up,
            DESCR_LCD_Backlight_Enabled,
            DESCR_Primary_Source_Interrupt_Alarm_Enabled,
            DESCR_Record_Fault_Code_Enabled,
        };
        for (const char *key : webOnlySwitchKeys)
        {
            purgeHaDiscoveryComponent(_mqtt, deviceId, "switch", key);
            purgeHaDiscoveryComponent(_mqtt, deviceId, "sensor", key);
            purgeHaDiscoveryComponent(_mqtt, deviceId, "binary_sensor", key);
            purgeHaDiscoveryComponent(_mqtt, deviceId, "number", key);
        }
    }
    else if (force)
    {
        // Pure MODBUS_POWMR must not retain PI-only controls from a previous
        // hybrid configuration.
        for (const char *key : piSwitchKeys)
        {
            purgeHaDiscoveryComponent(_mqtt, deviceId, "switch", key);
        }
    }
}

void MqttHandler::startHaDiscoverySweep()
{
    if (!_mqtt.connected() || !_settings.get.mqttHAEnabled())
    {
        return;
    }

    JsonDocument snapshot;
    _state.snapshotTo(snapshot);
    const char *protocol = snapshot["Status"]["protocol"] | "";
    _haDiscoverySweepPowMr = isPowMrProtocolName(protocol);

    _haDiscoverySweepTopic = String("homeassistant/+/") + getHaDeviceId() + "/+/config";
    if (_mqtt.subscribe(_haDiscoverySweepTopic.c_str()))
    {
        _haDiscoverySweepActive = true;
        _haDiscoverySweepStartedMs = millis();
        writeLog("[HA] Discovery cleanup sweep started");
    }
}

void MqttHandler::stopHaDiscoverySweep()
{
    if (!_haDiscoverySweepActive)
    {
        return;
    }

    if (_mqtt.connected() && _haDiscoverySweepTopic.length() > 0)
    {
        _mqtt.unsubscribe(_haDiscoverySweepTopic.c_str());
    }
    _haDiscoverySweepActive = false;
    _haDiscoverySweepTopic = "";
    writeLog("[HA] Discovery cleanup sweep finished");
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
