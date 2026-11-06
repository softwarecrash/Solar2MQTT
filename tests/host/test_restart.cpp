#include <Arduino.h>
#include "BatteryStorageMock.h"
#include <cassert>
#include <cmath>
#include <thread>
#include <vector>
#include <freertos/semphr.h>
#include <WiFiClientSecure.h>
#include "DownloadMock.h"
#define private public
#include "core/GitHubOtaUpdater.h"
#include "core/FirmwareWriter.h"
#include "core/EnergyBacklog.h"
#undef private

namespace battery_test
{
Storage storage;
}
FakeLittleFS LittleFS;
uint32_t testNow = 100;
FakeEsp ESP;
DownloadMock download;
FakeUpdate Update;
FirmwareWriter firmwareWriter;
#include "restart_generated.h"

static std::vector<std::string> events;
static unsigned warnings = 0;
void writeLog(const char *format, ...)
{
    if (std::strstr(format, "Checkpoint not verified before restart"))
        ++warnings;
}
// Exercise the actual checkpoint flush function with the real storage code.
class MqttHandler
{
  public:
    EnergyBacklog _energyBacklog;
    void flushPersistentEnergy();
    void refreshBatteryDiagnostics(unsigned long, bool)
    {
        events.push_back("flush");
    }
} mqttHandler;
#include "restart_flush_generated.h"
struct Inverter
{
    void shutdown()
    {
        events.push_back("shutdown");
    }
} inverterService;
struct RestartEsp
{
    void restart()
    {
        events.push_back("restart");
        throw 7;
    }
} restartEsp;
GitHubOtaUpdater otaUpdater("test", "test", "test", "test");
bool g_pendingRestart = false;
uint32_t g_restartAt = 0;
#define ESP restartEsp
#include "restart_main_generated.h"
#undef ESP

static void reset()
{
    battery_test::storage = battery_test::Storage{};
    download = DownloadMock{};
    Update = FakeUpdate{};
    // Each fixture models a fresh boot, clearing the committed writer latch.
    firmwareWriter.owner = nullptr;
    firmwareWriter.committed = false;
    testNow = 100;
    events.clear();
    warnings = 0;
    g_pendingRestart = false;
    otaUpdater._restartPending = false;
    otaUpdater._state = GitHubOtaUpdater::State::Idle;
    otaUpdater._assetUrl = "https://example.invalid/firmware.bin";
    otaUpdater.begin([]() { return true; });
    mqttHandler._energyBacklog = EnergyBacklog{};
    mqttHandler._energyBacklog.begin();
    mqttHandler._energyBacklog._batteryChargeWh = 12.125;
    mqttHandler._energyBacklog._batteryDischargeWh = 23.25;
    mqttHandler._energyBacklog._batteryDirty = true;
}
static void restartDue()
{
    bool restarted = false;
    try
    {
        processPendingRestart();
    }
    catch (int code)
    {
        assert(code == 7);
        restarted = true;
    }
    assert(restarted);
    assert((events == std::vector<std::string>{"flush", "shutdown", "restart"}));
}
static void successAndDeadlines()
{
    reset();
    processPendingRestart();
    assert(events.empty());
    otaUpdater.doDownload();
    assert(otaUpdater._state == GitHubOtaUpdater::State::Success && otaUpdater._restartPending);
    assert(events.empty());
    testNow = otaUpdater._restartAtMs - 1;
    processPendingRestart();
    assert(events.empty());
    ++testNow;
    restartDue();
    assert(!otaUpdater._restartPending);
    EnergyBacklog reboot;
    reboot.begin();
    assert(std::fabs(reboot._batteryChargeWh - 12.125) < 1e-9);
    assert(std::fabs(reboot._batteryDischargeWh - 23.25) < 1e-9);
    events.clear();
    processPendingRestart();
    assert(events.empty());
    reset();
    testNow = 0xfffffff0UL;
    otaUpdater.scheduleRestart(2500);
    testNow += 2499;
    processPendingRestart();
    assert(events.empty());
    ++testNow;
    restartDue();
    reset();
    g_pendingRestart = true;
    g_restartAt = testNow + 100;
    testNow += 99;
    processPendingRestart();
    assert(events.empty());
    ++testNow;
    restartDue();
    assert(!g_pendingRestart);
    reset();
    g_pendingRestart = true;
    g_restartAt = testNow;
    otaUpdater.scheduleRestart(0);
    restartDue();
    std::puts("PASS: successful GitHub download uses main-loop flush/shutdown/restart; exact "
              "deadline, once-only, API restart and timer wrap.");
}
static void failedDownloads()
{
    for (unsigned failure = 0; failure < 7; failure++)
    {
        reset();
        switch (failure)
        {
        case 0:
            otaUpdater._networkConnected = []() { return false; };
            break;
        case 1:
            otaUpdater._assetUrl = "";
            break;
        case 2:
            download.beginOk = false;
            break;
        case 3:
            download.code = 404;
            break;
        case 4:
            download.updateBeginOk = false;
            break;
        case 5:
            download.writeOk = false;
            break;
        case 6:
            download.endOk = false;
            break;
        }
        otaUpdater.doDownload();
        assert(otaUpdater._state == GitHubOtaUpdater::State::Error);
        assert(!otaUpdater._restartPending);
        testNow += 10000;
        processPendingRestart();
        assert(events.empty());
    }
    reset();
    otaUpdater.scheduleRestart(0);
    battery_test::storage.failFsWrites = battery_test::storage.failNvsWrites = true;
    restartDue();
    assert(warnings > 0); // existing policy: log failed checkpoint, then restart
    std::puts("PASS: network/URL/HTTP/begin/write/finalize failures never request reboot; failed "
              "checkpoint is reported.");
}
static void concurrentRequest()
{
    reset();
    std::atomic<bool> scheduled{false};
    std::atomic<unsigned> consumed{0};
    std::thread worker(
        [&]()
        {
            otaUpdater.scheduleRestart(0);
            scheduled = true;
        });
    std::thread reader(
        [&]()
        {
            while (!scheduled.load())
            {
            }
            if (otaUpdater.consumeRestartRequest(testNow))
                ++consumed;
        });
    worker.join();
    reader.join();
    if (otaUpdater.consumeRestartRequest(testNow))
        ++consumed;
    assert(consumed == 1);
    std::puts("PASS: asynchronous OTA request is consumed once under the production mutex.");
}
static void interruptedDownloads()
{
    for(unsigned fault=0;fault<6;++fault)
    {
        reset();
        switch(fault)
        {
        case 0: download.stalled=true; break;
        case 1: download.disconnectAt=1024; break;
        case 2: download.reportedSize=0; break;
        case 3: download.nullStream=true; break;
        case 4: download.reportedSize=4096; break;
        case 5: testNow=0xfffffff0UL;download.stalled=true;break;
        }
        const uint32_t started=testNow;
        otaUpdater.doDownload();
        assert(otaUpdater._state==GitHubOtaUpdater::State::Error&&!otaUpdater._restartPending);
        assert(uint32_t(testNow-started)<=15001UL);
        if(fault!=2&&fault!=3)assert(download.aborted==1);
    }
    reset();download.reportedSize=-1;otaUpdater.doDownload();
    assert(otaUpdater._state==GitHubOtaUpdater::State::Success&&otaUpdater._bytesDone==download.bytes.size());
    std::puts("PASS: stalled/truncated/empty/missing streams abort without restart; timeout wraps safely; unknown length completes.");
}
int main()
{
    successAndDeadlines();
    failedDownloads();
    interruptedDownloads();
    concurrentRequest();
    std::puts("Restart regressions passed.");
}
