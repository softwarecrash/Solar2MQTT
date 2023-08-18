/*
DALY2MQTT Project
https://github.com/softwarecrash/DALY2MQTT
*/
String htmlProcessor(const String &var)
{
    extern Settings settings;
    if (var == F("pre_head_template"))
        return (HTML_HEAD);
    if (var == F("pre_foot_template"))
        return (HTML_FOOT);
    if (var == F("pre_software_version"))
        return (SOFTWARE_VERSION);
    if (var == F("pre_swversion"))
        return (SWVERSION);
    if (var == F("pre_flash_size"))
        return (String(FlashSize).c_str());
    if (var == F("pre_esp01"))
        return (String(ESP01).c_str());
    if (var == F("pre_device_name"))
        return (settings.data.deviceName);
    if (var == F("pre_mqtt_server"))
        return (settings.data.mqttServer);
    if (var == F("pre_mqtt_port"))
        return (String(settings.data.mqttPort).c_str());
    if (var == F("pre_mqtt_user"))
        return (settings.data.mqttUser);
    if (var == F("pre_mqtt_pass"))
        return (settings.data.mqttPassword);
    if (var == F("pre_mqtt_topic"))
        return (settings.data.mqttTopic);
    if (var == F("pre_mqtt_refresh"))
        return (String(settings.data.mqttRefresh).c_str());
    if (var == F("pre_mqtt_json"))
        return (settings.data.mqttJson ? "checked" : "");
    if (var == F("pre_mqtt_mqtttrigger"))
        return (settings.data.mqttTriggerPath);
    if (var == F("pre_darkmode"))
        return (settings.data.webUIdarkmode ? "dark" : "light");
    if (var == F("pre_webuidarkmode"))
        return (settings.data.webUIdarkmode ? "checked" : "");
    return String();
}
