#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>

class WiFiManager;
class SolarState;
class SolarInverterService;
class MqttHandler;
class GitHubOtaUpdater;

class WebServerHandler
{
public:
    WebServerHandler(AsyncWebServer &server,
                     WiFiManager &wifiManager,
                     SolarState &state,
                     SolarInverterService &inverterService,
                     MqttHandler &mqttHandler,
                     GitHubOtaUpdater &otaUpdater);

    void begin();
    void setMqttConnected(bool value) { _mqttConnected = value; }
    void setInverterConnected(bool value) { _inverterConnected = value; }
    void notifyStatusBar();

private:
    static WebServerHandler *s_self;

    AsyncWebServer &_server;
    WiFiManager &_wifiManager;
    SolarState &_state;
    SolarInverterService &_inverterService;
    MqttHandler &_mqttHandler;
    GitHubOtaUpdater &_otaUpdater;
    AsyncWebSocket _wsStatus;
    Ticker _statusTicker;
    bool _mqttConnected;
    bool _inverterConnected;

    bool isAuthorized(AsyncWebServerRequest *request);
    void registerRoutes();
    void setupStatusWebSocket();
    void buildStatusJson(JsonDocument &doc);
    void sendAsset(AsyncWebServerRequest *request, const char *mime, const uint8_t *data, size_t len);
};
