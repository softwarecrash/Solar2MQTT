String htmlProcessor(const String &var)
{
    extern Settings settings;
    if (var == F("HEAD_TEMPLATE"))
        return (HTML_HEAD);
    if (var == F("FOOT_TEMPLATE"))
        return (HTML_FOOT);
    if (var == F("SOFTWARE_VERSION"))
        return (SOFTWARE_VERSION);
    if (var == F("SWVERSION"))
        return (SWVERSION);
    if (var == F("Flash_Size"))
        return (String(FlashSize).c_str());
//    if (var == F("ESP01"))
//        return (String(ESP01).c_str());
    if (var == F("DEVICE_NAME"))
        return (settings.data.deviceName);
    if (var == F("MQTT_SERVER"))
        return (settings.data.mqttServer);
    if (var == F("MQTT_PORT"))
        return (String(settings.data.mqttPort).c_str());
    if (var == F("MQTT_USER"))
        return (settings.data.mqttUser);
    if (var == F("MQTT_PASS"))
        return (settings.data.mqttPassword);
    if (var == F("MQTT_TOPIC"))
        return (settings.data.mqttTopic);
    if (var == F("MQTT_REFRESH"))
        return (String(settings.data.mqttRefresh).c_str());
//    if (var == F("MQTT_JSON"))
//        return (settings.data.mqttJson ? "checked":"");
    return String();
}
