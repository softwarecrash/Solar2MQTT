#include <Arduino.h>
#include "BatteryStorageMock.h"
#include <cassert>
#include <cmath>
#include <thread>
#include "descriptors.h"
#define private public
#include "core/SolarState.h"
#include "core/MqttHandler.h"
#undef private

namespace battery_test
{
Storage storage;
}
FakeLittleFS LittleFS;
uint32_t testNow = 100;
FakeEsp ESP;
void writeLog(const char *, ...) {}
class WiFiManager
{
};

// The API consumes a timing interface. It does not need the PI parser or
// custom-command scheduler; those have their own production-code tests.
class SolarInverterService
{
  public:
    BatterySampleTiming batterySampleTiming(unsigned long) const
    {
        return {};
    }
    bool hasCommandAnswer() const
    {
        return false;
    }
    String consumeCommandAnswer()
    {
        return "";
    }
};

struct TestSettings
{
    struct Values
    {
        bool mqttJson() const
        {
            return false;
        }
        bool mqttHAEnabled() const
        {
            return true;
        }
        const char *deviceName() const
        {
            return "Solar Test";
        }
    } get;
} _settings;

MqttHandler::MqttHandler(SolarState &state, WiFiManager &wifi, SolarInverterService &inverter)
    : _state(state), _wifiManager(wifi), _inverterService(inverter), _netClient(nullptr)
{
}
String MqttHandler::baseTopic() const
{
    return "solar/test";
}
void MqttHandler::triggerFullStatePublish()
{
    _pendingFullPublish = true;
}
#ifndef FULL_HA_DISCOVERY
void MqttHandler::publishHaEspInternalTemperature(JsonDocument &, JsonObjectConst,
                                                  std::vector<String> &, bool)
{
}
void MqttHandler::publishHaDs18b20(JsonDocument &, JsonObjectConst, std::vector<String> &, bool) {}
void MqttHandler::publishHaPowMrSettings(JsonDocument &, JsonObjectConst, std::vector<String> &,
                                         bool)
{
}
#endif
#include "mqtt_state_generated.h"

static void near(double actual, double expected)
{
    assert(std::isfinite(actual) && std::isfinite(expected));
    if (std::fabs(actual - expected) > 0.000001)
    {
        std::fprintf(stderr, "%.9f != %.9f\n", actual, expected);
        std::abort();
    }
}

struct Fixture
{
    SolarState state;
    WiFiManager wifi;
    SolarInverterService inverter;
    MqttHandler mqtt{state, wifi, inverter};

    explicit Fixture(protocol_type_t protocol)
    {
        state.begin();
        state.doc()["Status"]["protocol"] = protocolToString(protocol);
        state.commitLocalState();
    }

    ~Fixture()
    {
        vSemaphoreDelete(mqtt._batteryApiMutex);
        mqtt._batteryApiMutex = nullptr;
        vSemaphoreDelete(state._snapshotMutex);
        state._snapshotMutex = nullptr;
    }

    std::string publication(const char *key) const
    {
        const std::string topic = std::string("solar/test/LiveData/") + key;
        for (const auto &message : mqtt._mqtt.messages)
            if (message.topic == topic)
                return message.value;
        return "<absent>";
    }
};
#include "battery_api_cases.h"

#ifndef DISCOVERY_MEMORY_TEST
int main()
{
    battery_cases::apiAndMqtt();
    battery_cases::discoveryAfterMigration();
    std::puts("Battery API/discovery regressions passed.");
}
#endif
