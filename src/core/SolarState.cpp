#include "core/SolarState.h"

#include <esp_heap_caps.h>

JsonDocument g_stateDoc;
JsonObject deviceJson;
JsonObject staticData;
JsonObject liveData;

namespace
{
JsonObject ensureObject(JsonDocument &doc, const char *key)
{
    JsonVariant section = doc[key];
    if (section.isNull())
    {
        return section.to<JsonObject>();
    }
    return section.as<JsonObject>();
}

int heapFragmentationPercent()
{
    const size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap == 0)
    {
        return 0;
    }

    const size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const size_t effectiveLargestBlock = (largestBlock > freeHeap) ? freeHeap : largestBlock;
    return static_cast<int>(100 - ((effectiveLargestBlock * 100U) / freeHeap));
}

void copyObjectSection(JsonDocument &target, const char *key, JsonObjectConst source)
{
    JsonObject destination = target[key].to<JsonObject>();
    if (source.isNull())
    {
        return;
    }

    for (JsonPairConst entry : source)
    {
        destination[entry.key()] = entry.value();
    }
}

void appendSection(String &out, const char *name, JsonObjectConst object)
{
    out += "[";
    out += name;
    out += "]\n";
    for (JsonPairConst entry : object)
    {
        out += entry.key().c_str();
        out += "=";
        serializeJson(entry.value(), out);
        out += "\n";
    }
    out += "\n";
}
} // namespace

void SolarState::begin()
{
    g_stateDoc.clear();
    ensureObjects();
    status()["mqttConnected"] = false;
    status()["wifiConnected"] = false;
    status()["inverterConnected"] = false;
    status()["ethActive"] = false;
    status()["apMode"] = false;
}

JsonDocument &SolarState::doc()
{
    ensureObjects();
    return g_stateDoc;
}

void SolarState::refreshBindings()
{
    ensureObjects();
}

void SolarState::ensureObjects()
{
    deviceJson = ensureObject(g_stateDoc, "EspData");
    staticData = ensureObject(g_stateDoc, "DeviceData");
    liveData = ensureObject(g_stateDoc, "LiveData");
    ensureObject(g_stateDoc, "RawData");
    ensureObject(g_stateDoc, "Status");
}

JsonObject SolarState::raw()
{
    ensureObjects();
    return g_stateDoc["RawData"].as<JsonObject>();
}

JsonObject SolarState::status()
{
    ensureObjects();
    return g_stateDoc["Status"].as<JsonObject>();
}

void SolarState::snapshotTo(JsonDocument &target)
{
    ensureObjects();
    target.clear();
    copyObjectSection(target, "RawData", g_stateDoc["RawData"].as<JsonObjectConst>());
    copyObjectSection(target, "EspData", g_stateDoc["EspData"].as<JsonObjectConst>());
    copyObjectSection(target, "DeviceData", g_stateDoc["DeviceData"].as<JsonObjectConst>());
    copyObjectSection(target, "LiveData", g_stateDoc["LiveData"].as<JsonObjectConst>());
    copyObjectSection(target, "Status", g_stateDoc["Status"].as<JsonObjectConst>());
}

void SolarState::updateRuntime(const char *deviceName,
                               protocol_type_t protocol,
                               bool inverterConnected,
                               bool wifiConnected,
                               bool mqttConnected,
                               bool ethActive,
                               bool apMode,
                               int wifiRssi,
                               const String &ipAddress)
{
    ensureObjects();
    JsonDocument &doc = g_stateDoc;
    const char *networkType = ethActive ? "Ethernet" : (apMode ? "AP" : (wifiConnected ? "WiFi" : "Offline"));

    doc["EspData"]["Device_name"] = deviceName ? deviceName : "";
    doc["EspData"]["Wifi_RSSI"] = wifiRssi;
    doc["EspData"]["sw_version"] = STRVERSION;
    doc["EspData"]["Free_Heap"] = ESP.getFreeHeap();
    doc["EspData"]["HEAP_Fragmentation"] = heapFragmentationPercent();
    doc["EspData"]["runtime"] = millis() / 1000UL;
    doc["EspData"]["detect_protocol"] = static_cast<uint8_t>(protocol);
    doc["EspData"]["detect_protocol_name"] = protocolToString(protocol);
    doc["EspData"]["detect_protocol_raw_only"] = isRawOnlyPiProtocol(protocol);
    doc["EspData"]["detect_protocol_unknown"] = (protocol == PI30_UNKNOWN);
    doc["EspData"]["IP"] = ipAddress;
    doc["EspData"]["WiFiStatus"] = wifiConnected;
    doc["EspData"]["MQTTStatus"] = mqttConnected;
    doc["EspData"]["Ethernet_Active"] = ethActive;
    doc["EspData"]["Network_Type"] = networkType;
    doc["EspData"]["AP_Mode"] = apMode;
    doc["EspData"]["Inverter_Connected"] = inverterConnected;

    doc["Status"]["wifiConnected"] = wifiConnected;
    doc["Status"]["mqttConnected"] = mqttConnected;
    doc["Status"]["inverterConnected"] = inverterConnected;
    doc["Status"]["ethActive"] = ethActive;
    doc["Status"]["apMode"] = apMode;
    doc["Status"]["networkType"] = networkType;
    doc["Status"]["protocol"] = protocolToString(protocol);
    doc["Status"]["protocolRawOnly"] = isRawOnlyPiProtocol(protocol);
    doc["Status"]["protocolUnknown"] = (protocol == PI30_UNKNOWN);
    doc["Status"]["ip"] = ipAddress;

    refreshBindings();
}

void SolarState::updateRaw(const char *key, const String &value)
{
    g_stateDoc["RawData"][key] = value;
    if (strcmp(key, "QPI") == 0)
    {
        g_stateDoc["EspData"]["detect_raw_qpi"] = value;
    }

    refreshBindings();
}

String SolarState::buildDebugReport()
{
    JsonDocument snapshot;
    snapshotTo(snapshot);

    String out;
    out.reserve(4096);
    out += "Solar2MQTT Debug Report\n\n";
    appendSection(out, "RawData", snapshot["RawData"].as<JsonObjectConst>());
    appendSection(out, "EspData", snapshot["EspData"].as<JsonObjectConst>());
    appendSection(out, "DeviceData", snapshot["DeviceData"].as<JsonObjectConst>());
    appendSection(out, "LiveData", snapshot["LiveData"].as<JsonObjectConst>());
    appendSection(out, "Status", snapshot["Status"].as<JsonObjectConst>());
    return out;
}
