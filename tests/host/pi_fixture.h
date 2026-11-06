#pragma once
#include <Arduino.h>
#include "BatteryStorageMock.h"
#include <ArduinoJson.h>
#include <cassert>
#include <cmath>
#include <map>
#include <sstream>
#include <vector>
#include <thread>
#include "descriptors.h"
// Expose state only to the native regression harness. The production classes,
// scheduler, parsers, snapshot and publication functions are compiled unchanged.
#define private public
#include "PI_Serial/PI_Serial.h"
#include "core/SolarState.h"
#include "solar/SolarInverterService.h"
#include "core/MqttHandler.h"
#undef private

namespace battery_test
{
Storage storage;
}
FakeLittleFS LittleFS;
uint32_t testNow = 1;
FakeEsp ESP;
HardwareSerial testUart;
class WiFiManager
{
};
void writeLog(const char *, ...) {}
struct TestSettings
{
    struct Values
    {
        bool json = false;
        bool mqttJson() const
        {
            return json;
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
static std::map<std::string, std::string> replies;
static std::function<void(const std::string &)> whileWaiting;
static std::vector<std::string> requested;
static std::map<std::string, uint32_t> replyDelays;

String PI_Serial::requestData(String command)
{
    requested.push_back(command.text);
    if (whileWaiting)
        whileWaiting(command.text);
    const auto wait = replyDelays.find(command.text);
    testNow += wait == replyDelays.end() ? 80 : wait->second;
    const auto found = replies.find(command.text);
    const String reply = found == replies.end() ? String("NAK") : String(found->second);
    if (isValidResponse(reply))
        noteSuccessfulReply(command);
    return reply;
}
void PI_Serial::beginSerial(unsigned int baud)
{
    serialIntfBaud = baud;
}
void PI_Serial::autoDetect() {}
bool PI_Serial::pollPowMrPiSupplement()
{
    return true;
}
void PI_Serial::logStaticSummary() const {}
void PI_Serial::logDynamicSummary() const {}

#include "PI_Serial/QPI.h"
#include "PI_Serial/QPIRI.h"
#include "PI_Serial/QMN.h"
#include "PI_Serial/QFLAG.h"
#include "PI_Serial/QPIGS.h"
#include "PI_Serial/QPIGS2.h"
#include "PI_Serial/QMOD.h"
#include "PI_Serial/Q1.h"
#include "PI_Serial/QEX.h"
#include "PI_Serial/QPIWS.h"
#include "pi_state_generated.h"

SolarInverterService::SolarInverterService(SolarState &state)
    : _state(state), _serial(testUart), _client(nullptr), _simulationEnabled(false),
      _transportPaused(false)
{
}
SolarInverterService::~SolarInverterService() = default;
#include "service_state_generated.h"

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
void MqttHandler::publishHaEspInternalTemperature(JsonDocument &, JsonObjectConst,
                                                  std::vector<String> &, bool)
{
}
void MqttHandler::publishHaDs18b20(JsonDocument &, JsonObjectConst, std::vector<String> &, bool) {}
void MqttHandler::publishHaPowMrSettings(JsonDocument &, JsonObjectConst, std::vector<String> &,
                                         bool)
{
}
#include "mqtt_state_generated.h"

static const char *gkRating = "240.0 20.8 240.0 50.0 21.7 5000 4000 48.0 44.0 42.0 56.4 54.0 0 10 "
                              "080 0 2 3 6 20 0 0 48.0 0 1";
static const char *maxRating = "230.0 47.8 230.0 50.0 47.8 11000 11000 48.0 44.0 44.0 56.4 54.0 0 "
                               "002 080 0 2 3 6 01 0 0 48.0 0 1 480 0 150";
static const char *gkDay = "000.0 00.0 020.1 50.0 0000 0000 000 375 46.90 004 047 0029 0002 076.8 "
                           "46.90 00000 00010110 00 00 00093 010";
static const char *maxDay = "232.4 50.2 232.4 50.2 02440 02415 022 414 47.50 000 033 0041 01.8 "
                            "391.9 00.00 00000 00010110 00 00 00721 010 0 00 0000";
static const char *revoDay = "234.0 50.0 230.0 49.9 0000 0736 013 401 50.00 010 081 0026 0622 "
                             "311.0 53.00 16850 00000110 00 00 00622 010";
static const char *revoAll =
    "234 50.0 230 49.9 0736 013 51.2 098 101 000 289 15.3 1385 016850 000368 L 01 02";
static const char *legacyPv2 =
    "0002 086.4 46.70 00093 10000000 0000 0000 0000 000.0 00.00 00000 00177";
static const char *pi41Pv2 = "230.0 50.0 230.0 50.0 1000 0900 020 0005 081.6 47.20 00100100";
static std::string fields(const std::string &frame, size_t count, int replace = -1,
                          const char *value = "")
{
    std::istringstream input(frame);
    std::string out, field;
    for (size_t i = 0; i < count; i++)
    {
        assert(input >> field);
        if (i)
            out += ' ';
        out += static_cast<int>(i) == replace ? value : field;
    }
    return out;
}
static void near(double a, double b)
{
    if (!std::isfinite(a) || !std::isfinite(b) || std::fabs(a - b) > 0.000001)
    {
        std::fprintf(stderr, "%.9f != %.9f\n", a, b);
        std::abort();
    }
}

struct Fixture
{
    SolarState state;
    PI_Serial driver{testUart, 19, 23};
    WiFiManager wifi;
    SolarInverterService service{state};
    MqttHandler mqtt{state, wifi, service};
    unsigned completed = 0, dynamicCompleted = 0, snapshotChecks = 0;
    explicit Fixture(protocol_type_t protocol)
    {
        testNow = 1;
        whileWaiting = nullptr;
        requested.clear();
        replyDelays.clear();
        _settings.get.json = false;
        replies.clear();
        state.begin();
        driver.protocol = protocol;
        driver.forcedProtocol = protocol;
        driver.delayTime = protocol == PI30_MAX ? 100 : 1000;
        service._client = &driver;
        driver.requestCallback = [this]
        {
            if (driver.hasCompleteStateUpdate())
            {
                ++completed;
                if (!driver.requestStaticData)
                    ++dynamicCompleted;
            }
            service.handleClientUpdate();
        };
        replies["QPI"] = protocol == PI41 ? "PI41" : "PI30";
        replies["QPIRI"] = protocol == PI30_MAX ? maxRating : gkRating;
        replies["QMN"] = "NAK";
        replies["QFLAG"] = "EbjylDakuvxz";
        replies["QALL"] = "NAK";
        replies["QMOD"] = "B";
        replies["QPIGS"] = protocol == PI30_MAX ? maxDay : gkDay;
        replies["QPIGS2"] = protocol == PI30_PIP_GK ? legacyPv2 : "01.9 388.9 00745";
        replies["QPIWS"] =
            protocol == PI30_MAX ? std::string(36, '0') : "00000100001000000000000000000000000000";
        if (protocol == PI41)
        {
            replies["QPIGS"] = fields(gkDay, 21, 12, "04.0");
            replies["QPIGS2"] = pi41Pv2;
            replies["QPIWS"] = std::string(32, '0');
        }
        if (protocol == PI30_REVO)
        {
            replies["QPIRI"] = fields(gkRating, 21);
            replies["QPIGS"] = revoDay;
            replies["QALL"] = revoAll;
            replies["QPIWS"] = "NAK";
        }
    }
    ~Fixture()
    {
        vSemaphoreDelete(state._snapshotMutex);
        state._snapshotMutex = nullptr;
    }
    void poll()
    {
        testNow += driver.delayTime + 1;
        driver.loop();
        checkSnapshot();
    }
    void cycles(unsigned n)
    {
        const auto target = dynamicCompleted + n;
        for (unsigned guard = 0; dynamicCompleted < target && guard < 250; guard++)
            poll();
        assert(dynamicCompleted >= target);
    }
    JsonDocument snapshot()
    {
        JsonDocument d;
        state.snapshotTo(d);
        return d;
    }
    std::string publication(const char *key)
    {
        const std::string topic = std::string("solar/test/LiveData/") + key;
        for (const auto &m : mqtt._mqtt.messages)
            if (m.topic == topic)
                return m.value;
        return "<absent>";
    }
    void checkSnapshot()
    {
        ++snapshotChecks;
        auto data = snapshot();
        mqtt._mqtt.messages.clear();
        mqtt.publishState();
        const auto live = data["LiveData"].as<JsonObjectConst>();
        const std::string frame = data["RawData"]["QPIGS2"] | "";
        if ((frame == legacyPv2 || frame == "01.9 388.9 00745" || frame == pi41Pv2) &&
            live[DESCR_PV2_Input_Current].isNull())
        {
            std::string debug;
            serializeJson(data, debug);
            std::fprintf(stderr, "Incomplete committed sample protocol=%d step=%u at=%lu: %s\n",
                         driver.protocol, driver.requestCounter, millis(), debug.c_str());
            std::string working;
            serializeJson(g_stateDoc, working);
            std::fprintf(stderr, "Working state: %s\n", working.c_str());
        }
        if (frame == legacyPv2)
        {
            near(live[DESCR_PV2_Input_Current].as<double>(), 1.08);
            near(live[DESCR_PV2_Input_Power].as<double>(), 93);
            near(live[DESCR_PV_Total_Input_Power].as<double>(), 177);
        }
        else if (frame == "01.9 388.9 00745")
        {
            near(live[DESCR_PV2_Input_Current].as<double>(), 1.9);
            near(live[DESCR_PV2_Input_Power].as<double>(), 739);
        }
        else if (frame == pi41Pv2)
        {
            near(live[DESCR_PV2_Input_Current].as<double>(), 5);
            near(live[DESCR_PV2_Input_Power].as<double>(), 408);
        }
        else
            return;
        assert(!live[DESCR_PV2_Input_Voltage].isNull());
        assert(publication(DESCR_PV2_Input_Power) != "<absent>");
        assert(publication(DESCR_PV2_Input_Voltage) != "<absent>");
        assert(publication("PV1_Input_Power") == "<absent>");
        assert(publication("PV1_Input_Voltage") == "<absent>");
        const std::string warningFrame = data["RawData"]["QPIWS"] | "";
        const std::string qallFrame = data["RawData"]["QALL"] | "";
        const bool warningSource = !warningFrame.empty() && warningFrame != "NAK" &&
                                   warningFrame != "NOA" && warningFrame != "ERCRC";
        const bool revoWarningSource = driver.protocol == PI30_REVO && !qallFrame.empty() &&
                                       qallFrame != "NAK" && qallFrame != "NOA" &&
                                       qallFrame != "ERCRC";
        if (dynamicCompleted > 0 && (warningSource || revoWarningSource))
        {
            assert(!live[DESCR_Fault_Code].isNull());
            assert(publication(DESCR_Fault_Code) != "<absent>");
        }
    }
};
