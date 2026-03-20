#pragma once

#include "pins.h"

#define SETTINGS_ITEMS(X) \
  X(STRING, "network", "deviceName", deviceName, "Solar2MQTT", 0, 0) \
  X(STRING, "network", "wifiSsid0", wifiSsid0, "", 0, 0) \
  X(STRING, "network", "wifiPassword0", wifiPassword0, "", 0, 0) \
  X(STRING, "network", "bssid0", wifiBssid0, "", 0, 0) \
  X(BOOL,   "network", "bssidLock", wifiBssidLock, false, 0, 1) \
  X(STRING, "network", "wifiSsid1", wifiSsid1, "", 0, 0) \
  X(STRING, "network", "wifiPassword1", wifiPassword1, "", 0, 0) \
  X(STRING, "network", "bssid1", wifiBssid1, "", 0, 0) \
  X(STRING, "network", "staticIP", staticIP, "", 0, 0) \
  X(STRING, "network", "staticGW", staticGW, "", 0, 0) \
  X(STRING, "network", "staticSN", staticSN, "", 0, 0) \
  X(STRING, "network", "staticDNS", staticDNS, "", 0, 0) \
  X(STRING, "network", "webUIuser", webUIuser, "", 0, 0) \
  X(STRING, "network", "webUIPassword", webUIPassword, "", 0, 0) \
  X(BOOL,   "mqtt", "SSLenabled", mqttSSL, false, 0, 1) \
  X(STRING, "mqtt", "host", mqttHost, "", 0, 0) \
  X(STRING, "mqtt", "user", mqttUser, "", 0, 0) \
  X(STRING, "mqtt", "password", mqttPassword, "", 0, 0) \
  X(UINT16, "mqtt", "port", mqttPort, 1883, 1, 65535) \
  X(STRING, "mqtt", "topic", mqttTopic, "Solar", 0, 0) \
  X(UINT16, "mqtt", "refresh", mqttRefresh, 30, 0, 86400) \
  X(BOOL,   "mqtt", "jsonMode", mqttJson, false, 0, 1) \
  X(BOOL,   "mqtt", "HAEnabled", mqttHAEnabled, false, 0, 1) \
  X(STRING, "mqtt", "triggerTopic", mqttTriggerPath, "", 0, 0) \
  X(INT32,  "device", "uartRx", inverterRxPin, PIN_INVERTER_RX_DEFAULT, -1, 48) \
  X(INT32,  "device", "uartTx", inverterTxPin, PIN_INVERTER_TX_DEFAULT, -1, 48) \
  X(INT32,  "device", "uartDir", inverterDirPin, PIN_INVERTER_DE_DEFAULT, -1, 48) \
  X(INT32,  "device", "statusLedPin", statusLedPin, PIN_LED_STATUS, -1, 48) \
  X(UINT16, "device", "statusLedBrightness", statusLedBrightness, 128, 0, 255) \
  X(UINT32, "device", "pollIntervalMs", pollIntervalMs, 100, 25, 5000)
