#include "core/WiFiManager.h"

#include "pins.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ctype.h>

#if HAS_LAN
#include <ETH.h>
#endif

#include "core/LogSerial.h"
#include "core/SettingsPrefs.h"

extern Settings _settings;

namespace
{
DNSServer dnsServer;
IPAddress apIp(192, 168, 4, 1);

#if HAS_LAN
bool s_ethConnected = false;

const char *currentNetworkHostName()
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

void onNetworkEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_ETH_START:
        LogSerial.println(F("[Network] ETH started"));
        ETH.setHostname(currentNetworkHostName());
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        LogSerial.println(F("[Network] ETH connected (PHY link)"));
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        s_ethConnected = true;
        LogSerial.printf("[Network] ETH got IP: %s\n", ETH.localIP().toString().c_str());
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        s_ethConnected = false;
        LogSerial.println(F("[Network] ETH disconnected"));
        break;
    case ARDUINO_EVENT_ETH_STOP:
        s_ethConnected = false;
        LogSerial.println(F("[Network] ETH stopped"));
        break;
    default:
        break;
    }
}

String resolveEthIpString()
{
    const IPAddress ethIp = ETH.localIP();
    if (ETH.linkUp() && ethIp != IPAddress(0, 0, 0, 0))
    {
        return ethIp.toString();
    }

    return "";
}
#endif

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

WiFiManager::WiFiManager(AsyncWebServer &server)
    : _server(server),
      _isApMode(false),
      _ethActive(false)
{
}

void WiFiManager::begin()
{
#if HAS_LAN
    WiFi.onEvent(onNetworkEvent);
#endif

    applySavedNetworkConfig();
    refreshMdns();
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

    if (isEthActive())
    {
        if (_isApMode)
        {
            LogSerial.println(F("[Network] Ethernet active, stopping AP mode"));
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            _isApMode = false;
        }
        return;
    }

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
        LogSerial.println(F("[Network] Lost STA connection, switching to AP"));
        startApMode();
        _isApMode = true;
    }
    else if (connectToWifi())
    {
        LogSerial.println(F("[Network] Reconnected to STA"));
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        _isApMode = false;
    }
}

bool WiFiManager::getConnectionState() const
{
    return isEthActive() || WiFi.status() == WL_CONNECTED;
}

void WiFiManager::reconfigure()
{
    LogSerial.println(F("[Network] Reconfiguring network"));
    dnsServer.stop();
    WiFi.scanDelete();

#if HAS_LAN
    if (_ethActive)
    {
        ETH.end();
        s_ethConnected = false;
        _ethActive = false;
    }
#endif

    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    _isApMode = false;
    delay(50);

    applySavedNetworkConfig();
    refreshMdns();
}

void WiFiManager::refreshMdns()
{
    MDNS.end();
    if (MDNS.begin(networkHostName()))
    {
        MDNS.addService("http", "tcp", 80);
        LogSerial.printf("[Network] mDNS started: http://%s.local (IP: %s)\n",
                         networkHostName(),
                         ipAddress().c_str());
    }
}

bool WiFiManager::isInApMode() const
{
    return _isApMode;
}

bool WiFiManager::isEthActive() const
{
#if HAS_LAN
    return _ethActive && s_ethConnected && ETH.linkUp() && ETH.localIP() != IPAddress(0, 0, 0, 0);
#else
    return false;
#endif
}

bool WiFiManager::hasLanSupport() const
{
#if HAS_LAN
    return true;
#else
    return false;
#endif
}

bool WiFiManager::ethernetEnabled() const
{
#if HAS_LAN
    return _settings.get.ethEnabled();
#else
    return false;
#endif
}

int WiFiManager::rssi() const
{
    return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
}

String WiFiManager::ipAddress() const
{
#if HAS_LAN
    const String ethIp = resolveEthIpString();
    if (ethIp.length() > 0)
    {
        return ethIp;
    }
#endif

    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }

    return WiFi.softAPIP().toString();
}

bool WiFiManager::initEthernet()
{
#if !HAS_LAN
    return false;
#else
    if (!_settings.get.ethEnabled())
    {
        LogSerial.println(F("[Network] Ethernet disabled in settings"));
        _ethActive = false;
        return false;
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
        ETH.config(ip, gw, sn, dns);
        LogSerial.printf("[Network] ETH static IP %s / GW %s / SN %s / DNS %s\n",
                         ip.toString().c_str(),
                         gw.toString().c_str(),
                         sn.toString().c_str(),
                         dns.toString().c_str());
    }
    else
    {
        LogSerial.println(F("[Network] ETH using DHCP"));
    }

    ETH.setHostname(networkHostName());
    s_ethConnected = false;

#if defined(ETH_SPI_W5500) && ETH_SPI_W5500
    LogSerial.println(F("[Network] Initializing Ethernet (W5500 SPI)..."));
    const bool ok = ETH.begin(ETH_PHY_W5500,
                              ETH_PHY_ADDR_AUTO,
                              PIN_ETH_SPI_CS,
                              PIN_ETH_SPI_INT,
                              PIN_ETH_SPI_RST,
                              SPI2_HOST,
                              PIN_ETH_SPI_SCLK,
                              PIN_ETH_SPI_MISO,
                              PIN_ETH_SPI_MOSI);
#else
    LogSerial.println(F("[Network] Initializing Ethernet (LAN8720)..."));
    const bool ok = ETH.begin(ETH_PHY_LAN8720,
                              PIN_ETH_PHY_ADDR,
                              PIN_ETH_MDC,
                              PIN_ETH_MDIO,
                              PIN_ETH_POWER,
                              ETH_CLOCK_GPIO0_IN);
#endif

    if (!ok)
    {
        LogSerial.println(F("[Network] Ethernet init failed, falling back to WiFi"));
        _ethActive = false;
        return false;
    }

    _ethActive = true;

    const uint32_t start = millis();
    while ((millis() - start) < 10000UL)
    {
        if (isEthActive())
        {
            LogSerial.printf("[Network] Ethernet ready, IP: %s\n", ipAddress().c_str());
            return true;
        }
        delay(100);
    }

    LogSerial.println(F("[Network] Ethernet IP timeout, falling back to WiFi"));
    return false;
#endif
}

bool WiFiManager::connectToWifi()
{
    if (strlen(_settings.get.wifiSsid0()) == 0 && strlen(_settings.get.wifiSsid1()) == 0)
    {
        LogSerial.println(F("[Network] No SSIDs configured"));
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
        const bool bssidLockRequested = _settings.get.wifiBssidLock();
        const bool lockBssid = bssidLockRequested && parseBssid(candidate.bssid, bssid);

        LogSerial.printf("[Network] Trying SSID: %s\n", candidate.ssid);
        if (bssidLockRequested && !lockBssid)
        {
            LogSerial.println(F("[Network] BSSID lock requested, but stored BSSID is missing/invalid. Falling back to strongest AP for the SSID."));
        }

        if (lockBssid)
        {
            WiFi.setScanMethod(WIFI_FAST_SCAN);
            WiFi.begin(candidate.ssid, candidate.password, 0, bssid, true);
        }
        else
        {
            WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
            WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
            WiFi.begin(candidate.ssid, candidate.password);
        }

        for (int attempt = 0; attempt < 12; ++attempt)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                LogSerial.printf("[Network] WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
                return true;
            }
            delay(500);
        }
    }

    return false;
}

bool WiFiManager::applySavedNetworkConfig()
{
    bool haveEthernet = false;

#if HAS_LAN
    haveEthernet = initEthernet();
#endif

    if (!haveEthernet && !connectToWifi())
    {
        startApMode();
        _isApMode = true;
        return false;
    }

    _isApMode = false;
    return true;
}

void WiFiManager::startApMode()
{
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
    WiFi.softAP(String(SOURCE_NAME) + "-AP");
    dnsServer.start(53, "*", apIp);
    LogSerial.printf("[Network] AP started on %s\n", WiFi.softAPIP().toString().c_str());
}

const char *WiFiManager::networkHostName() const
{
#if HAS_LAN
    return currentNetworkHostName();
#else
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
#endif
}
