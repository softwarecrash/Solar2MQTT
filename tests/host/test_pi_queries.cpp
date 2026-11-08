#include <ArduinoJson.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class String
{
public:
    String(const char *input = "") : value(input) {}
    explicit String(const std::string &input) : value(input) {}
    bool operator==(const char *other) const { return value == other; }
    bool startsWith(const char *prefix) const { return value.rfind(prefix, 0) == 0; }
    bool isEmpty() const { return value.empty(); }
    size_t length() const { return value.size(); }
    const char *c_str() const { return value.c_str(); }
    String substring(size_t start) const { return String(value.substr(start)); }
    void trim()
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        value = first == std::string::npos ? "" : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    }

private:
    std::string value;
};

using SemaphoreHandle_t = std::mutex *;
constexpr unsigned long portMAX_DELAY = 0;
void xSemaphoreTake(SemaphoreHandle_t mutex, unsigned long) { mutex->lock(); }
void xSemaphoreGive(SemaphoreHandle_t mutex) { mutex->unlock(); }
void writeLog(const char *, ...) {}

#include "pi_query_helpers.h"

static unsigned long now = 1;
unsigned long millis() { return now; }
JsonDocument device;
JsonObject staticData, liveData;

struct Modbus
{
    bool connection = false;
    void loop() {}
    bool consumePowMrLivePassCompleted() { return false; }
    String requestData(const String &) { return "MODBUS"; }
};

class PI_Serial
{
public:
    protocol_type_t protocol = PI30;
    std::atomic_bool suspendSerial{false};
    unsigned long delayTime = 100, nextDetectAt = 0, previousTime = 0, lastSuccessfulDynamicCycleAt = 0;
    unsigned requestCounter = 0, connectionCounter = 0;
    bool requestStaticData = true, connection = false, cycleHadSuccessfulReply = false, cycleBackupActive = false;
    bool customQuerySlot = false, lastCustomCommandWasQuery = false;
    bool powMrPiFlagRefreshRequested = false, powMrPiEqualizationRefreshRequested = false;
    unsigned serialIntfBaud = 9600;
    const char *startChar = "#", *delimiter = ",";
    String customCommandBuffer;
    std::mutex commandMutex;
    SemaphoreHandle_t customCommandMutex = &commandMutex;
    struct { struct { String commandAnswer; } raw; } get;
    JsonDocument cycleLiveBackup, cycleStaticBackup;
    std::function<void()> requestCallback;
    std::function<void()> onInquiry;
    Modbus modbusStub;
    Modbus *modbus = &modbusStub;
    std::vector<std::string> commands;
    bool failStatic = false, failDynamic = false;
    unsigned staticPolls = 0, livePolls = 0;

    void autoDetect() {}
    void refineProtocol() {}
    void logStaticSummary() {}
    void logDynamicSummary() {}
    void beginSerial(unsigned int) {}
    bool pollPowMrPiSupplement() { return true; }
    bool requestUnsupportedPiStatic() { return true; }
    bool requestUnsupportedPiDynamic() { return true; }
    bool PIXX_QPIRI()
    {
        if (!requestStaticData)
            return dynamicReply();
        ++staticPolls;
        staticData["rating"] = 4000;
        cycleHadSuccessfulReply = !failStatic;
        return !failStatic;
    }
    bool PIXX_QMN() { return true; }
    bool PIXX_QPI() { return true; }
    bool PIXX_QFLAG() { return true; }
    bool PIXX_QPIGS() { return dynamicReply(); }
    bool PIXX_QPIGS2() { return true; }
    bool PIXX_QMOD() { return true; }
    bool PIXX_Q1() { return true; }
    bool PIXX_QEX() { return true; }
    bool PIXX_QPIWS() { return true; }
    bool dynamicReply()
    {
        liveData["reading"] = ++livePolls;
        cycleHadSuccessfulReply = !failDynamic;
        return !failDynamic;
    }
    String requestData(const String &command)
    {
        commands.emplace_back(command.c_str());
        if (onInquiry)
            onInquiry();
        return "NAK";
    }
    bool loop();
    bool isModbus();
    bool sendCustomCommand();
    static bool isReadOnlyPiQuery(const String &command);
    String sendCommand(String command);
    void beginCycleBackup();
    void restoreCycleBackup();
    void clearCycleBackup();
    void markSuccessfulDynamicCycle();
};

#include "pi_query_methods.h"

static void reset()
{
    now = 1;
    device.clear();
    auto root = device.to<JsonObject>();
    liveData = root["live"].to<JsonObject>();
    staticData = root["static"].to<JsonObject>();
    liveData["reading"] = 0;
    staticData["rating"] = 1;
}

static void tick(PI_Serial &driver, unsigned count = 1)
{
    while (count-- > 0)
    {
        now += 101;
        assert(driver.loop());
    }
}

static void scheduledQueries(protocol_type_t protocol)
{
    reset();
    PI_Serial driver;
    driver.protocol = protocol;
    driver.sendCommand("Q1");
    tick(driver, 4);
    assert(driver.commands.empty() && !driver.requestStaticData);
    tick(driver, 8);
    assert(driver.commands.empty() && driver.connection && driver.lastSuccessfulDynamicCycleAt > 0);

    const auto firstSample = driver.lastSuccessfulDynamicCycleAt;
    const int reading = liveData["reading"];
    driver.onInquiry = [&] { driver.sendCommand("QED20261105"); };
    tick(driver);
    driver.onInquiry = {};
    assert(driver.commands.size() == 1 && driver.commands.back() == "Q1");
    assert(driver.get.raw.commandAnswer == "NAK" && driver.lastSuccessfulDynamicCycleAt == firstSample);
    assert(liveData["reading"] == reading && !driver.requestStaticData);

    tick(driver, 7);
    assert(driver.commands.size() == 1 && driver.cycleBackupActive);
    tick(driver);
    assert(driver.lastSuccessfulDynamicCycleAt > firstSample && driver.commands.size() == 1);
    const auto nextSample = driver.lastSuccessfulDynamicCycleAt;
    tick(driver);
    assert(driver.commands.size() == 2 && driver.commands.back() == "QED20261105");
    assert(driver.lastSuccessfulDynamicCycleAt == nextSample && driver.staticPolls == 1);

    // Unknown, malformed-date and control commands keep the original refresh path.
    for (const char *command : {"QUNKNOWN", "QED2026x105", "QED202611", "POP02"})
    {
        driver.requestStaticData = false;
        driver.sendCommand(command);
        tick(driver);
        assert(driver.commands.back() == command && driver.requestStaticData && driver.requestCounter == 0);
    }
}

static void failedPollingAllowsOneInquiry()
{
    reset();
    PI_Serial driver;
    driver.failStatic = true;
    driver.sendCommand("QPI");
    tick(driver);
    assert(driver.staticPolls == 1 && driver.commands.empty());
    tick(driver);
    assert(driver.commands.size() == 1 && driver.requestStaticData && !driver.connection);
    assert(staticData["rating"] == 1 && driver.lastSuccessfulDynamicCycleAt == 0);

    driver.sendCommand("QVFW4");
    tick(driver);
    assert(driver.staticPolls == 2 && driver.commands.size() == 1);
    tick(driver);
    assert(driver.commands.size() == 2 && driver.commands.back() == "QVFW4");

    driver.failStatic = false;
    tick(driver, 4);
    driver.failDynamic = true;
    driver.sendCommand("QPIGS");
    tick(driver);
    assert(driver.livePolls == 1 && driver.commands.size() == 2 && liveData["reading"] == 0);
    tick(driver);
    assert(driver.commands.size() == 3 && driver.commands.back() == "QPIGS");
    assert(!driver.requestStaticData && !driver.connection && driver.lastSuccessfulDynamicCycleAt == 0);
    driver.failDynamic = false;
    tick(driver, 8);
    assert(driver.connection && driver.lastSuccessfulDynamicCycleAt > 0);
}

static void legacyPaths()
{
    for (auto protocol : {PI15, PI18, MODBUS_POWMR, MODBUS_POWMR_PI})
    {
        reset();
        PI_Serial driver;
        driver.protocol = protocol;
        driver.requestStaticData = false;
        driver.sendCommand("Q1");
        tick(driver);
        assert(driver.requestStaticData && driver.requestCounter == 0);
        assert(driver.get.raw.commandAnswer == (driver.isModbus() ? "MODBUS" : "NAK"));
    }

    reset();
    PI_Serial driver;
    driver.protocol = MODBUS_POWMR_PI;
    driver.sendCommand("powmr pi PEa");
    tick(driver);
    assert(driver.commands.size() == 1 && driver.commands.front() == "PEa");
    assert(driver.protocol == MODBUS_POWMR_PI && driver.serialIntfBaud == 9600 && driver.powMrPiFlagRefreshRequested);
}

int main()
{
    for (auto protocol : {PI16, PI30, PI30_UNKNOWN, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        scheduledQueries(protocol);
    failedPollingAllowsOneInquiry();
    legacyPaths();
    std::puts("PI inquiries: complete cycles, one query per attempt, recovery, queue concurrency and legacy paths passed");
}
