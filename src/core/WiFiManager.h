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
    bool isEthActive() const;
    bool hasLanSupport() const;
    bool ethernetEnabled() const;
    int rssi() const;
    String ipAddress() const;

private:
    AsyncWebServer &_server;
    bool _isApMode;
    bool _ethActive;

    bool connectToWifi();
    void startApMode();
    bool initEthernet();
    const char *networkHostName() const;
};
