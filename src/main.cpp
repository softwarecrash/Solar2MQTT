#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "core/Ds18b20Service.h"
#include "core/FactoryResetManager.h"
#include "core/GitHubOtaUpdater.h"
#include "core/LogSerial.h"
#include "core/MqttHandler.h"
#include "core/SettingsPrefs.h"
#include "core/SolarState.h"
#include "core/StatusLedService.h"
#include "core/WebServerHandler.h"
#include "core/WiFiManager.h"
#include "main.h"
#include "pins.h"
#include "solar/SolarInverterService.h"

Settings _settings;

#ifndef OTA_GITHUB_OWNER
#error "OTA_GITHUB_OWNER must be defined via build flags"
#endif

#ifndef OTA_GITHUB_REPO
#error "OTA_GITHUB_REPO must be defined via build flags"
#endif

#ifndef BUILD_VARIANT
#error "BUILD_VARIANT must be defined via build flags"
#endif

bool g_pendingRestart = false;
uint32_t g_restartAt = 0;
bool g_pendingFactoryReset = false;
uint32_t g_factoryResetAt = 0;
bool g_pendingInverterReconfigure = false;
uint32_t g_inverterReconfigureAt = 0;

SolarState solarState;
AsyncWebServer server(80);
WiFiManager wifiManager(server);
SolarInverterService inverterService(solarState);
Ds18b20Service ds18b20Service;
MqttHandler mqttHandler(solarState, wifiManager, inverterService);
StatusLedService statusLedService;
GitHubOtaUpdater otaUpdater(OTA_GITHUB_OWNER, OTA_GITHUB_REPO, STRVERSION, BUILD_VARIANT);
WebServerHandler webServerHandler(server, wifiManager, solarState, inverterService, mqttHandler, otaUpdater);

namespace
{
void updateRuntimeState()
{
    solarState.updateRuntime(_settings.get.deviceName(),
                             inverterService.protocol(),
                             inverterService.isConnected(),
                             wifiManager.getConnectionState(),
                             mqttHandler.isConnected(),
                             wifiManager.isInApMode(),
                             wifiManager.rssi(),
                             wifiManager.ipAddress());
    solarState.refreshBindings();

    JsonObject statusObject = solarState.status();
    statusObject["loopbackRunning"] = inverterService.loopbackRunning();
    statusObject["loopbackDone"] = inverterService.loopbackDone();
    statusObject["loopbackOk"] = inverterService.loopbackOk();
    statusObject["loopbackMessage"] = inverterService.loopbackMessage();
    statusObject["simulationEnabled"] = inverterService.simulationEnabled();
    statusObject["simulationProtocol"] = inverterService.simulationEnabled() ? "PI30" : "";
    solarState.doc()["EspData"]["Simulation_Active"] = inverterService.simulationEnabled();
    solarState.doc()["EspData"]["Simulation_Mode"] = inverterService.simulationEnabled() ? "PI30" : "";
    solarState.refreshBindings();
}
} // namespace

void setup()
{
    LogSerial.begin(MONITOR_SPEED);
    delay(150);

    _settings.begin();
    solarState.begin();
    statusLedService.begin(_settings.get.statusLedPin(),
                           static_cast<uint8_t>(_settings.get.statusLedBrightness()));

    FactoryResetManager::begin(10000, 6);
    wifiManager.begin();
    otaUpdater.begin();

    inverterService.setCallback([]()
                                {
        updateRuntimeState();
        mqttHandler.triggerFullStatePublish();
        webServerHandler.notifyStatusBar(); });
    inverterService.begin();

    ds18b20Service.setCallback([](uint8_t index, float temperature)
                               {
        mqttHandler.publishSensorImmediate(index, temperature);
        mqttHandler.triggerFullStatePublish(); });
    ds18b20Service.begin(PIN_DS18B20, deviceJson);

    mqttHandler.begin();
    webServerHandler.begin();

    updateRuntimeState();
    webServerHandler.setMqttConnected(mqttHandler.isConnected());
    webServerHandler.setInverterConnected(inverterService.isConnected());
    webServerHandler.notifyStatusBar();
}

void loop()
{
    FactoryResetManager::loop();
    wifiManager.loop();
    inverterService.loop();
    ds18b20Service.loop();
    mqttHandler.loop();
    webServerHandler.loop();

    if (g_pendingInverterReconfigure && static_cast<int32_t>(millis() - g_inverterReconfigureAt) >= 0)
    {
        g_pendingInverterReconfigure = false;
        inverterService.reconfigure();
        statusLedService.configure(_settings.get.statusLedPin(),
                                   static_cast<uint8_t>(_settings.get.statusLedBrightness()));
        mqttHandler.triggerFullStatePublish();
    }

    if (g_pendingFactoryReset && static_cast<int32_t>(millis() - g_factoryResetAt) >= 0)
    {
        g_pendingFactoryReset = false;
        FactoryResetManager::requestFactoryReset();
    }

    if (g_pendingRestart && static_cast<int32_t>(millis() - g_restartAt) >= 0)
    {
        g_pendingRestart = false;
        ESP.restart();
    }

    updateRuntimeState();
    webServerHandler.setMqttConnected(mqttHandler.isConnected());
    webServerHandler.setInverterConnected(inverterService.isConnected());
    webServerHandler.loop();
    statusLedService.loop(wifiManager.getConnectionState(),
                          strlen(_settings.get.mqttHost()) > 0,
                          mqttHandler.isConnected(),
                          inverterService.isConnected());
}
