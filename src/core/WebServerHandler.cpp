#include "core/WebServerHandler.h"

#include <ArduinoJson.h>
#include <Update.h>
#include <WiFi.h>

#include "core/GitHubOtaUpdater.h"
#include "core/LogSerial.h"
#include "core/MqttHandler.h"
#include "core/SettingsPrefs.h"
#include "core/SolarState.h"
#include "core/WiFiManager.h"
#include "main.h"
#include "solar/SolarInverterService.h"
#include "www.h"

extern Settings _settings;

namespace
{
String s_restoreBody;

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

namespace NetScanCache
{
uint32_t s_cacheTs = 0;
String s_cacheJson;
constexpr uint32_t kCacheMs = 10000;

bool cacheValid()
{
    if (s_cacheTs == 0 || s_cacheJson.isEmpty())
    {
        return false;
    }

    return (millis() - s_cacheTs) < kCacheMs;
}

void startAsyncScanIfNeeded(bool force)
{
    if (!force && cacheValid())
    {
        return;
    }

    const int scanState = WiFi.scanComplete();
    if (scanState == WIFI_SCAN_RUNNING)
    {
        return;
    }

    if (force)
    {
        if (scanState >= 0)
        {
            WiFi.scanDelete();
        }

        s_cacheJson = "";
        s_cacheTs = 0;
        (void)WiFi.scanNetworks(true, true);
        return;
    }

    if (scanState < 0)
    {
        (void)WiFi.scanNetworks(true, true);
    }
}

void collectIfFinished()
{
    const int count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING || count < 0)
    {
        return;
    }

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int index = 0; index < count; index++)
    {
        JsonObject entry = networks.add<JsonObject>();
        entry["ssid"] = WiFi.SSID(index);
        entry["rssi"] = WiFi.RSSI(index);
        entry["enc"] = (WiFi.encryptionType(index) != WIFI_AUTH_OPEN);
        entry["bssid"] = WiFi.BSSIDstr(index);
    }

    WiFi.scanDelete();

    s_cacheJson = "";
    serializeJson(doc, s_cacheJson);
    s_cacheTs = millis();
}

const String &json()
{
    return s_cacheJson;
}
} // namespace NetScanCache
}

WebServerHandler *WebServerHandler::s_self = nullptr;

WebServerHandler::WebServerHandler(AsyncWebServer &server,
                                   WiFiManager &wifiManager,
                                   SolarState &state,
                                   SolarInverterService &inverterService,
                                   MqttHandler &mqttHandler,
                                   GitHubOtaUpdater &otaUpdater)
    : _server(server),
      _wifiManager(wifiManager),
      _state(state),
      _inverterService(inverterService),
      _mqttHandler(mqttHandler),
      _otaUpdater(otaUpdater),
      _wsStatus("/ws-status"),
      _mqttConnected(false),
      _inverterConnected(false)
{
    s_self = this;
}

void WebServerHandler::begin()
{
    registerRoutes();
    setupStatusWebSocket();
    LogSerial.begin(&_server, MONITOR_SPEED, 256);
    _server.begin();
    _statusTicker.attach_ms(1000, []()
                            {
        if (WebServerHandler::s_self != nullptr)
        {
            WebServerHandler::s_self->notifyStatusBar();
        } });
}

void WebServerHandler::notifyStatusBar()
{
    if (_inverterService.isBusy())
    {
        return;
    }

    JsonDocument doc;
    buildStatusJson(doc);
    _lastStatusPayload = "";
    serializeJson(doc, _lastStatusPayload);
    _wsStatus.textAll(_lastStatusPayload);
}

bool WebServerHandler::isAuthorized(AsyncWebServerRequest *request)
{
    if (_wifiManager.isInApMode())
    {
        return true;
    }

    if (!*_settings.get.webUIuser())
    {
        return true;
    }

    return request->authenticate(_settings.get.webUIuser(), _settings.get.webUIPassword());
}

void WebServerHandler::registerRoutes()
{
    auto serveAsset = [this](const char *path, const char *mime, const uint8_t *data, size_t len)
    {
        _server.on(path, HTTP_GET, [this, mime, data, len](AsyncWebServerRequest *request)
                   {
            if (!isAuthorized(request))
            {
                return request->requestAuthentication();
            }
            sendAsset(request, mime, data, len); });
    };

    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        if (_wifiManager.isInApMode())
        {
            sendAsset(request, wifi_html_gz_mime, wifi_html_gz, wifi_html_gz_len);
            return;
        }

        sendAsset(request, index_html_gz_mime, index_html_gz, index_html_gz_len); });
    serveAsset("/status", index_html_gz_mime, index_html_gz, index_html_gz_len);
    serveAsset("/menu", menu_html_gz_mime, menu_html_gz, menu_html_gz_len);
    serveAsset("/settings", menu_html_gz_mime, menu_html_gz, menu_html_gz_len);
    serveAsset("/wifi", wifi_html_gz_mime, wifi_html_gz, wifi_html_gz_len);
    serveAsset("/wifisetup", wifi_html_gz_mime, wifi_html_gz, wifi_html_gz_len);
    serveAsset("/mqtt", mqtt_html_gz_mime, mqtt_html_gz, mqtt_html_gz_len);
    serveAsset("/mqttsettings", mqtt_html_gz_mime, mqtt_html_gz, mqtt_html_gz_len);
    serveAsset("/device", device_html_gz_mime, device_html_gz, device_html_gz_len);
    serveAsset("/settingsedit", device_html_gz_mime, device_html_gz, device_html_gz_len);
    serveAsset("/devicesettings", device_html_gz_mime, device_html_gz, device_html_gz_len);
    serveAsset("/firmware", firmware_html_gz_mime, firmware_html_gz, firmware_html_gz_len);
    serveAsset("/firmwareupdate", firmware_html_gz_mime, firmware_html_gz, firmware_html_gz_len);
    serveAsset("/debug", debug_html_gz_mime, debug_html_gz, debug_html_gz_len);
    serveAsset("/confirmreset", confirmreset_html_gz_mime, confirmreset_html_gz, confirmreset_html_gz_len);
    serveAsset("/reboot", reboot_html_gz_mime, reboot_html_gz, reboot_html_gz_len);
    serveAsset("/webserial", webserial_html_gz_mime, webserial_html_gz, webserial_html_gz_len);
    serveAsset("/app.js", app_js_gz_mime, app_js_gz, app_js_gz_len);
    serveAsset("/style.css", style_css_gz_mime, style_css_gz, style_css_gz_len);
    serveAsset("/backgroundCanvas.js", backgroundCanvas_js_gz_mime, backgroundCanvas_js_gz, backgroundCanvas_js_gz_len);
    serveAsset("/statusbar.js", statusbar_js_gz_mime, statusbar_js_gz, statusbar_js_gz_len);
    serveAsset("/favicon.svg", favicon_svg_gz_mime, favicon_svg_gz, favicon_svg_gz_len);
    serveAsset("/logo.svg", logo_svg_gz_mime, logo_svg_gz, logo_svg_gz_len);
    serveAsset("/solar2mqtt.png", solar2mqtt_png_gz_mime, solar2mqtt_png_gz, solar2mqtt_png_gz_len);

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        if (_inverterService.isBusy() && _lastStatusPayload.length() > 0)
        {
            request->send(200, "application/json", _lastStatusPayload);
            return;
        }

        JsonDocument doc;
        buildStatusJson(doc);
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json); });

    _server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument snapshot;
        _state.snapshotTo(snapshot);
        String json;
        serializeJson(snapshot, json);
        request->send(200, "application/json", json); });

    _server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;
        JsonObject network = doc["network"].to<JsonObject>();
        network["deviceName"] = _settings.get.deviceName();
        network["wifiSsid0"] = _settings.get.wifiSsid0();
        network["wifiPassword0"] = _settings.get.wifiPassword0();
        network["wifiBssid0"] = _settings.get.wifiBssid0();
        network["wifiSsid1"] = _settings.get.wifiSsid1();
        network["wifiPassword1"] = _settings.get.wifiPassword1();
        network["wifiBssid1"] = _settings.get.wifiBssid1();
        network["wifiBssidLock"] = _settings.get.wifiBssidLock();
        network["staticIP"] = _settings.get.staticIP();
        network["staticGW"] = _settings.get.staticGW();
        network["staticSN"] = _settings.get.staticSN();
        network["staticDNS"] = _settings.get.staticDNS();
        network["webUIuser"] = _settings.get.webUIuser();
        network["webUIPassword"] = _settings.get.webUIPassword();

        JsonObject mqtt = doc["mqtt"].to<JsonObject>();
        mqtt["host"] = _settings.get.mqttHost();
        mqtt["user"] = _settings.get.mqttUser();
        mqtt["password"] = _settings.get.mqttPassword();
        mqtt["port"] = _settings.get.mqttPort();
        mqtt["topic"] = _settings.get.mqttTopic();
        mqtt["refresh"] = _settings.get.mqttRefresh();
        mqtt["ssl"] = _settings.get.mqttSSL();
        mqtt["jsonMode"] = _settings.get.mqttJson();
        mqtt["ha"] = _settings.get.mqttHAEnabled();
        mqtt["triggerTopic"] = _settings.get.mqttTriggerPath();

        JsonObject device = doc["device"].to<JsonObject>();
        device["uartRx"] = _settings.get.inverterRxPin();
        device["uartTx"] = _settings.get.inverterTxPin();
        device["uartDir"] = _settings.get.inverterDirPin();
        device["pollIntervalMs"] = _settings.get.pollIntervalMs();

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json); });

    _server.on("/netconf.json", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!_wifiManager.isInApMode() && !isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;
        doc["deviceName"] = _settings.get.deviceName();
        doc["ssid0"] = _settings.get.wifiSsid0();
        doc["pass0"] = _settings.get.wifiPassword0();
        doc["bssid0"] = _settings.get.wifiBssid0();
        doc["bssidLock"] = _settings.get.wifiBssidLock();
        doc["ssid1"] = _settings.get.wifiSsid1();
        doc["pass1"] = _settings.get.wifiPassword1();
        doc["bssid1"] = _settings.get.wifiBssid1();

        IPAddress ip;
        IPAddress subnet;
        IPAddress gateway;
        IPAddress dns;
        doc["ip"] = (ip.fromString(_settings.get.staticIP()) && ip != IPAddress(0, 0, 0, 0)) ? _settings.get.staticIP() : "";
        doc["subnet"] = (subnet.fromString(_settings.get.staticSN()) && subnet != IPAddress(0, 0, 0, 0)) ? _settings.get.staticSN() : "";
        doc["gateway"] = (gateway.fromString(_settings.get.staticGW()) && gateway != IPAddress(0, 0, 0, 0)) ? _settings.get.staticGW() : "";
        doc["dns"] = (dns.fromString(_settings.get.staticDNS()) && dns != IPAddress(0, 0, 0, 0)) ? _settings.get.staticDNS() : "";

        doc["webUser"] = _settings.get.webUIuser();
        doc["webPass"] = _settings.get.webUIPassword();

        String json;
        serializeJson(doc, json);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Cache-Control", "no-store");
        request->send(response); });

    _server.on("/netlist", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!_wifiManager.isInApMode() && !isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        const bool force = request->hasParam("force");
        NetScanCache::startAsyncScanIfNeeded(force);
        NetScanCache::collectIfFinished();

        const char *emptyPayload = "{\"networks\":[]}";
        AsyncWebServerResponse *response = nullptr;
        if (NetScanCache::cacheValid())
        {
            response = request->beginResponse(200, "application/json", NetScanCache::json());
        }
        else
        {
            response = request->beginResponse(200, "application/json", emptyPayload);
        }

        response->addHeader("Cache-Control", "no-store");
        request->send(response); });

    _server.on("/api/settings/network", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        for (int i = 0; i < request->params(); ++i)
        {
            const AsyncWebParameter *param = request->getParam(i);
            const String &name = param->name();
            const String &value = param->value();

            if (name == "deviceName") _settings.set.deviceName(value);
            else if (name == "wifiSsid0") _settings.set.wifiSsid0(value);
            else if (name == "wifiPassword0") _settings.set.wifiPassword0(value);
            else if (name == "wifiBssid0") _settings.set.wifiBssid0(value);
            else if (name == "wifiSsid1") _settings.set.wifiSsid1(value);
            else if (name == "wifiPassword1") _settings.set.wifiPassword1(value);
            else if (name == "wifiBssid1") _settings.set.wifiBssid1(value);
            else if (name == "wifiBssidLock") _settings.set.wifiBssidLock(value.toInt() != 0);
            else if (name == "staticIP") _settings.set.staticIP(value);
            else if (name == "staticGW") _settings.set.staticGW(value);
            else if (name == "staticSN") _settings.set.staticSN(value);
            else if (name == "staticDNS") _settings.set.staticDNS(value);
            else if (name == "webUIuser") _settings.set.webUIuser(value);
            else if (name == "webUIPassword") _settings.set.webUIPassword(value);
        }

        _settings.save();
        g_pendingRestart = true;
        g_restartAt = millis() + 1000;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/submitConfig", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!_wifiManager.isInApMode() && !isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        for (int i = 0; i < request->params(); ++i)
        {
            const AsyncWebParameter *param = request->getParam(i);
            const String &name = param->name();
            const String &value = param->value();

            if (name == "devicename") _settings.set.deviceName(value);
            else if (name == "ssid0") _settings.set.wifiSsid0(value);
            else if (name == "password0") _settings.set.wifiPassword0(value);
            else if (name == "bssid0") _settings.set.wifiBssid0(value);
            else if (name == "bssidLock") _settings.set.wifiBssidLock(value.toInt() != 0);
            else if (name == "ssid1") _settings.set.wifiSsid1(value);
            else if (name == "password1") _settings.set.wifiPassword1(value);
            else if (name == "bssid1") _settings.set.wifiBssid1(value);
            else if (name == "ip") _settings.set.staticIP(value);
            else if (name == "subnet") _settings.set.staticSN(value);
            else if (name == "gateway") _settings.set.staticGW(value);
            else if (name == "dns") _settings.set.staticDNS(value);
            else if (name == "webUser") _settings.set.webUIuser(value);
            else if (name == "webPass") _settings.set.webUIPassword(value);
        }

        _settings.save();
        g_pendingRestart = true;
        g_restartAt = millis() + 1000;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/settings/mqtt", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        for (int i = 0; i < request->params(); ++i)
        {
            const AsyncWebParameter *param = request->getParam(i);
            const String &name = param->name();
            const String &value = param->value();

            if (name == "host") _settings.set.mqttHost(value);
            else if (name == "user") _settings.set.mqttUser(value);
            else if (name == "password") _settings.set.mqttPassword(value);
            else if (name == "topic") _settings.set.mqttTopic(value);
            else if (name == "triggerTopic") _settings.set.mqttTriggerPath(value);
            else if (name == "port") _settings.set.mqttPort(static_cast<uint16_t>(value.toInt()));
            else if (name == "refresh") _settings.set.mqttRefresh(static_cast<uint16_t>(value.toInt()));
            else if (name == "ssl") _settings.set.mqttSSL(value.toInt() != 0);
            else if (name == "jsonMode") _settings.set.mqttJson(value.toInt() != 0);
            else if (name == "ha") _settings.set.mqttHAEnabled(value.toInt() != 0);
        }

        _settings.save();
        g_pendingRestart = true;
        g_restartAt = millis() + 1000;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/settings/device", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        for (int i = 0; i < request->params(); ++i)
        {
            const AsyncWebParameter *param = request->getParam(i);
            const String &name = param->name();
            const String &value = param->value();

            if (name == "uartRx") _settings.set.inverterRxPin(value.toInt());
            else if (name == "uartTx") _settings.set.inverterTxPin(value.toInt());
            else if (name == "uartDir") _settings.set.inverterDirPin(value.toInt());
            else if (name == "pollIntervalMs") _settings.set.pollIntervalMs(value.toInt());
        }

        _settings.save();
        g_pendingInverterReconfigure = true;
        g_inverterReconfigureAt = millis() + 50;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/command", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        String command;
        if (request->hasParam("command", true))
        {
            command = request->getParam("command", true)->value();
        }
        else if (request->hasParam("command"))
        {
            command = request->getParam("command")->value();
        }

        _inverterService.queueCommand(command);
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/mqtt/discovery", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }
        _mqttHandler.triggerHaDiscovery();
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }
        g_pendingRestart = true;
        g_restartAt = millis() + 500;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }
        g_pendingFactoryReset = true;
        g_factoryResetAt = millis() + 500;
        request->send(200, "application/json", "{\"success\":true}"); });

    _server.on("/api/loopback/start", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }
        const bool started = _inverterService.requestLoopback();
        request->send(started ? 200 : 409,
                      "application/json",
                      started ? "{\"success\":true}" : "{\"success\":false}"); });

    _server.on("/api/loopback/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;
        doc["running"] = _inverterService.loopbackRunning();
        doc["done"] = _inverterService.loopbackDone();
        doc["ok"] = _inverterService.loopbackOk();
        doc["message"] = _inverterService.loopbackMessage();
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json); });

    _server.on("/api/debug/report", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }
        const String report = _state.buildDebugReport();
        request->send(200, "text/plain", report); });

    _server.on("/api/settings/backup", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        bool pretty = false;
        if (request->hasParam("pretty"))
        {
            pretty = request->getParam("pretty")->value().toInt() != 0;
        }

        request->send(200, "application/json", _settings.backup(pretty)); });

    _server.on(
        "/api/settings/restore",
        HTTP_POST,
        [this](AsyncWebServerRequest *request)
        {
            if (!isAuthorized(request))
            {
                return request->requestAuthentication();
            }
        },
        nullptr,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            if (!isAuthorized(request))
            {
                request->requestAuthentication();
                return;
            }

            static bool merge = true;
            static bool saveAfter = true;

            if (index == 0)
            {
                s_restoreBody = "";
                s_restoreBody.reserve(total);
                merge = true;
                saveAfter = true;

                if (request->hasParam("merge"))
                {
                    merge = request->getParam("merge")->value().toInt() != 0;
                }
                if (request->hasParam("save"))
                {
                    saveAfter = request->getParam("save")->value().toInt() != 0;
                }
            }

            s_restoreBody.concat(reinterpret_cast<const char *>(data), len);
            if (index + len < total)
            {
                return;
            }

            if (!s_restoreBody.length())
            {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Empty request body\"}");
                return;
            }

            const bool ok = _settings.restore(s_restoreBody, merge, saveAfter);
            if (!ok)
            {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Failed to parse or restore settings\"}");
                return;
            }

            if (saveAfter)
            {
                g_pendingRestart = true;
                g_restartAt = millis() + 2000;
            }

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Settings restored\"}");
        });

    _server.on("/api/ping", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        JsonDocument doc;
        doc["ok"] = true;
        doc["uptimeMs"] = millis();
        doc["restartPending"] = g_pendingRestart;
        if (g_pendingRestart)
        {
            int32_t remaining = static_cast<int32_t>(g_restartAt) - static_cast<int32_t>(millis());
            if (remaining < 0)
            {
                remaining = 0;
            }
            doc["restartInMs"] = remaining;
        }

        String json;
        serializeJson(doc, json);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Cache-Control", "no-store");
        request->send(response); });

    auto finishUpdate = [this](AsyncWebServerRequest *request)
    {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        if (Update.hasError())
        {
            request->send(500, "application/json", "{\"success\":false}");
            return;
        }

        g_pendingRestart = true;
        g_restartAt = millis() + 500;
        request->send(200, "application/json", "{\"success\":true}");
    };

    auto uploadUpdate = [this](AsyncWebServerRequest *request,
                               const String &filename,
                               size_t index,
                               uint8_t *data,
                               size_t len,
                               bool final)
    {
        if (!isAuthorized(request))
        {
            return;
        }

        if (index == 0)
        {
            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            {
                Update.printError(Serial);
            }
        }

        if (len > 0 && Update.write(data, len) != len)
        {
            Update.printError(Serial);
        }

        if (final && !Update.end(true))
        {
            Update.printError(Serial);
        }
    };

    _server.on("/update_firmware", HTTP_POST, finishUpdate, uploadUpdate);
    _server.on("/update", HTTP_POST, finishUpdate, uploadUpdate);

    _server.on("/ota/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        const String json = _otaUpdater.statusJson();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Cache-Control", "no-store");
        request->send(response); });

    _server.on("/ota/check", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        _otaUpdater.requestCheck();
        const String json = _otaUpdater.statusJson();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Cache-Control", "no-store");
        request->send(response); });

    _server.on("/ota/update", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
        if (!isAuthorized(request))
        {
            return request->requestAuthentication();
        }

        _otaUpdater.startUpdate();
        const String json = _otaUpdater.statusJson();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Cache-Control", "no-store");
        request->send(response); });
}

void WebServerHandler::setupStatusWebSocket()
{
    _wsStatus.onEvent([this](AsyncWebSocket *, AsyncWebSocketClient *client, AwsEventType type, void *, uint8_t *, size_t)
                      {
        if (type == WS_EVT_CONNECT)
        {
            if (_inverterService.isBusy() && _lastStatusPayload.length() > 0)
            {
                client->text(_lastStatusPayload);
                return;
            }

            JsonDocument doc;
            buildStatusJson(doc);
            String payload;
            serializeJson(doc, payload);
            _lastStatusPayload = payload;
            client->text(payload);
        } });

    _server.addHandler(&_wsStatus);
}

void WebServerHandler::buildStatusJson(JsonDocument &doc)
{
    const bool wifiConnected = _wifiManager.getConnectionState();
    const bool apMode = _wifiManager.isInApMode();
    const int wifiRssi = _wifiManager.rssi();
    const String ip = _wifiManager.ipAddress();

    doc["mqttConnected"] = _mqttConnected;
    doc["inverterConnected"] = _inverterConnected;
    doc["wifiConnected"] = wifiConnected;
    doc["apMode"] = apMode;
    doc["ip"] = ip;
    doc["protocol"] = _inverterService.protocolName();
    doc["simulation"] = _inverterService.simulationEnabled();
    doc["simulationProtocol"] = _inverterService.simulationEnabled() ? "PI30" : "";
    doc["deviceName"] = _state.doc()["EspData"]["Device_name"];
    doc["fw"] = _state.doc()["EspData"]["sw_version"];
    doc["wifi"]["connected"] = wifiConnected;
    doc["wifi"]["apMode"] = apMode;
    doc["wifi"]["rssi"] = wifiRssi;
    doc["mqtt"] = _mqttConnected;
    doc["inverter"] = _inverterConnected;
    doc["service"]["apMode"] = apMode;
    doc["service"]["loopbackRunning"] = _inverterService.loopbackRunning();
    doc["service"]["loopbackOk"] = _inverterService.loopbackOk();
    doc["service"]["simulationEnabled"] = _inverterService.simulationEnabled();
    doc["loopback"]["running"] = _inverterService.loopbackRunning();
    doc["loopback"]["done"] = _inverterService.loopbackDone();
    doc["loopback"]["ok"] = _inverterService.loopbackOk();
    doc["loopback"]["message"] = _inverterService.loopbackMessage();
    copyObjectSection(doc, "EspData", _state.doc()["EspData"].as<JsonObjectConst>());
    copyObjectSection(doc, "DeviceData", _state.doc()["DeviceData"].as<JsonObjectConst>());
    copyObjectSection(doc, "LiveData", _state.doc()["LiveData"].as<JsonObjectConst>());
}

void WebServerHandler::sendAsset(AsyncWebServerRequest *request, const char *mime, const uint8_t *data, size_t len)
{
    AsyncWebServerResponse *response = request->beginResponse(200, mime, data, len);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}
