#include "core/WiFiManager.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ctype.h>

#include "core/LogSerial.h"
#include "core/SettingsPrefs.h"

extern Settings _settings;

namespace
{
DNSServer dnsServer;
IPAddress apIp(192, 168, 4, 1);

int hexNibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

bool parseBssid(const char *value, uint8_t out[6])
{
    if (!value || !*value || !out)
    {
        return false;
    }

    size_t pos = 0;
    for (int i = 0; i < 6; ++i)
    {
        const int hi = hexNibble(value[pos]);
        const int lo = hexNibble(value[pos + 1]);
        if (hi < 0 || lo < 0)
        {
            return false;
        }

        out[i] = static_cast<uint8_t>((hi << 4) | lo);
        pos += 2;

        if (i < 5)
        {
            if (value[pos] != ':')
            {
                return false;
            }
            pos++;
        }
    }

    return value[pos] == '\0';
}
} // namespace

WiFiManager::WiFiManager(AsyncWebServer &server) : _server(server), _isApMode(false) {}

void WiFiManager::begin()
{
    if (!connectToWifi())
    {
        startApMode();
        _isApMode = true;
    }

    if (MDNS.begin(networkHostName()))
    {
        MDNS.addService("http", "tcp", 80);
        LogSerial.printf("[WiFi] mDNS started: http://%s.local\n", networkHostName());
    }
}

void WiFiManager::loop()
{
    if (_isApMode)
    {
        dnsServer.processNextRequest();
    }

    static unsigned long lastCheck = 0;
    if ((millis() - lastCheck) < 10000UL)
    {
        return;
    }
    lastCheck = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
        if (_isApMode)
        {
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            _isApMode = false;
        }
        return;
    }

    if (!_isApMode)
    {
        LogSerial.println(F("[WiFi] Lost STA connection, switching to AP"));
        startApMode();
        _isApMode = true;
    }
    else if (connectToWifi())
    {
        LogSerial.println(F("[WiFi] Reconnected to STA"));
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        _isApMode = false;
    }
}

bool WiFiManager::getConnectionState() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::isInApMode() const
{
    return _isApMode;
}

int WiFiManager::rssi() const
{
    return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
}

String WiFiManager::ipAddress() const
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

bool WiFiManager::connectToWifi()
{
    if (strlen(_settings.get.wifiSsid0()) == 0 && strlen(_settings.get.wifiSsid1()) == 0)
    {
        LogSerial.println(F("[WiFi] No SSIDs configured"));
        return false;
    }

    WiFi.persistent(false);
    WiFi.mode(_isApMode ? WIFI_AP_STA : WIFI_STA);
    WiFi.setHostname(networkHostName());
    WiFi.setSleep(false);
    WiFi.disconnect();

    struct Candidate
    {
        const char *ssid;
        const char *password;
        const char *bssid;
    };

    const Candidate candidates[] = {
        {_settings.get.wifiSsid0(), _settings.get.wifiPassword0(), _settings.get.wifiBssid0()},
        {_settings.get.wifiSsid1(), _settings.get.wifiPassword1(), _settings.get.wifiBssid1()},
    };

    for (const Candidate &candidate : candidates)
    {
        if (!candidate.ssid || !candidate.ssid[0])
        {
            continue;
        }

        IPAddress ip;
        IPAddress gw;
        IPAddress sn;
        IPAddress dns;
        const bool useStatic =
            ip.fromString(_settings.get.staticIP()) &&
            gw.fromString(_settings.get.staticGW()) &&
            sn.fromString(_settings.get.staticSN()) &&
            dns.fromString(_settings.get.staticDNS()) &&
            ip != IPAddress(0, 0, 0, 0);

        if (useStatic)
        {
            WiFi.config(ip, gw, sn, dns);
        }

        uint8_t bssid[6] = {};
        const bool lockBssid = _settings.get.wifiBssidLock() && parseBssid(candidate.bssid, bssid);

        LogSerial.printf("[WiFi] Trying SSID: %s\n", candidate.ssid);
        if (lockBssid)
        {
            WiFi.begin(candidate.ssid, candidate.password, 0, bssid, true);
        }
        else
        {
            WiFi.begin(candidate.ssid, candidate.password);
        }

        for (int attempt = 0; attempt < 12; ++attempt)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                LogSerial.printf("[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
                return true;
            }
            delay(500);
        }
    }

    return false;
}

void WiFiManager::startApMode()
{
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
    WiFi.softAP(String(SOURCE_NAME) + "-AP");
    dnsServer.start(53, "*", apIp);
    LogSerial.printf("[WiFi] AP started on %s\n", WiFi.softAPIP().toString().c_str());
}

const char *WiFiManager::networkHostName() const
{
    static char host[64];
    const char *source = _settings.get.deviceName();
    if (!source || !*source)
    {
        source = SOURCE_NAME;
    }

    size_t index = 0;
    bool lastDash = false;
    while (*source && index < sizeof(host) - 1)
    {
        const unsigned char c = static_cast<unsigned char>(*source++);
        if (isalnum(c))
        {
            host[index++] = static_cast<char>(tolower(c));
            lastDash = false;
        }
        else if (!lastDash && index > 0)
        {
            host[index++] = '-';
            lastDash = true;
        }
    }

    while (index > 0 && host[index - 1] == '-')
    {
        index--;
    }

    if (index == 0)
    {
        strcpy(host, "solar2mqtt");
        return host;
    }

    host[index] = '\0';
    return host;
}
