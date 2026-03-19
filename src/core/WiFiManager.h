#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class WiFiManager
{
public:
    explicit WiFiManager(AsyncWebServer &server);

    void begin();
    void loop();

    bool getConnectionState() const;
    bool isInApMode() const;
    int rssi() const;
    String ipAddress() const;

private:
    AsyncWebServer &_server;
    bool _isApMode;

    bool connectToWifi();
    void startApMode();
    const char *networkHostName() const;
};
