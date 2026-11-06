#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/semphr.h>

#include "solar/SolarTypes.h"

static constexpr size_t SOLAR_STATE_DOC_SIZE = 24576;

extern JsonDocument g_stateDoc;
extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

class SolarState
{
public:
    void begin();

    JsonDocument &doc();
    void refreshBindings();
    JsonObject raw();
    JsonObject status();
    bool snapshotTo(JsonDocument &target);
    bool serializeSnapshot(String &output);
    bool commitInverterState();
    bool commitLocalState();
    bool recoverWorkingState();
    void resetInverterState();

    void updateRuntime(const char *deviceName,
                       protocol_type_t protocol,
                       bool inverterConnected,
                       bool wifiConnected,
                       bool mqttConnected,
                       bool ethActive,
                       bool apMode,
                       int wifiRssi,
                       const String &ipAddress);

    void updateRaw(const char *key, const String &value);
    void writeDebugReport(Print &out);
    String buildDebugReport();

private:
    void ensureObjects();
    JsonDocument _inverterSnapshot;
    JsonDocument _localSnapshot;
    SemaphoreHandle_t _snapshotMutex = nullptr;
};
