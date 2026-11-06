#include <Arduino.h>
#include <cassert>
#include <fstream>
#include <iterator>
#include <thread>
#include <vector>
#include <freertos/semphr.h>
#define private public
#include "core/GitHubOtaUpdater.h"
#undef private
uint32_t testNow = 100;
FakeEsp ESP;
OtaMock ota;
TestUpdate Update;
std::vector<std::unique_ptr<TestTask>> tasks;
static std::string release;
#ifdef TEST_WRAP_ALLOC
static bool allocationFault = false;
static size_t allocationIndex = 0, failAllocation = SIZE_MAX;
struct Allocation
{
    void *pointer;
    size_t size;
};
static Allocation allocationSizes[512];
static size_t allocationSizeCount = 0;
static void recordAllocation(void *p, size_t n)
{
    if (!p)
        return;
    for (size_t i = 0; i < allocationSizeCount; ++i)
        if (allocationSizes[i].pointer == p)
        {
            allocationSizes[i].size = n;
            return;
        }
    assert(allocationSizeCount < 512);
    allocationSizes[allocationSizeCount++] = {p, n};
}
static size_t allocationSize(void *p)
{
    for (size_t i = 0; i < allocationSizeCount; ++i)
        if (allocationSizes[i].pointer == p)
            return allocationSizes[i].size;
    return 0;
}
extern "C" void *__real_malloc(size_t);
extern "C" void *__real_realloc(void *, size_t);
extern "C" void *__wrap_malloc(size_t n)
{
    if (allocationFault && allocationIndex++ == failAllocation)
        return nullptr;
    void *p = __real_malloc(n);
    if (allocationFault)
        recordAllocation(p, n);
    return p;
}
// ArduinoJson relies on shrinking realloc succeeding. Inject failures in
// allocations/growth, as on the ESP heap, rather than breaking that contract.
extern "C" void *__wrap_realloc(void *p, size_t n)
{
    if (allocationFault && n > allocationSize(p) && allocationIndex++ == failAllocation)
        return nullptr;
    void *next = __real_realloc(p, n);
    if (allocationFault)
        recordAllocation(next, n);
    return next;
}
#endif
static void finishTasks()
{
    for (auto &task : tasks)
        if (task->worker.joinable())
            task->worker.join();
    tasks.clear();
}
static void reset()
{
    finishTasks();
    ota = OtaMock{};
    ota.response = release;
    testNow = 100;
}
static void dispose(GitHubOtaUpdater &u)
{
    finishTasks();
    if (u._lockHandle)
    {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(u._lockHandle));
        u._lockHandle = nullptr;
    }
}
static JsonDocument status(GitHubOtaUpdater &u)
{
    JsonDocument d;
    assert(!deserializeJson(d, u.statusJson().c_str()));
    return d;
}
static void actualRelease(bool streaming)
{
    reset();
    ota.largeStringUnavailable = streaming;
    GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
    u.begin([] { return true; });
    u.doCheck();
    auto d = status(u);
    assert(d["state"] == "update_available");
    assert(d["latest"] == "2.0.4");
    assert(d["asset"] == "Solar2MQTT_wemos_d1_mini32_V2.0.4.bin.ota");
    assert(ota.url == "https://api.github.com/repos/softwarecrash/Solar2MQTT/releases/latest");
    assert(ota.updateBegins == 0 && ota.closed && ota.http10);
    if (streaming)
        assert(ota.getStringCalls == 0 && ota.streamCalls > 0);
    dispose(u);
}
static void versions()
{
    assert(GitHubOtaUpdater::compareVersions("V2.0.4", "0.0.0-dev-2.0.6-RC1") < 0);
    assert(GitHubOtaUpdater::compareVersions("V2.0.4", "V2.0.3") > 0);
    assert(GitHubOtaUpdater::compareVersions("V2.0.4", "2.0.4") == 0);
    assert(GitHubOtaUpdater::compareVersions("2.0.6", "2.0.6-RC1") > 0);
    assert(GitHubOtaUpdater::compareVersions("2.0.14A", "2.0.14") > 0);
    assert(GitHubOtaUpdater::compareVersions("2.0.14B", "2.0.14A") > 0);
    reset();
    GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "0.0.0-dev-2.0.6-RC1", "wemos_d1_mini32");
    u.begin([] { return true; });
    u.doCheck();
    assert(status(u)["state"] == "up_to_date" && !u.startUpdate() && ota.updateBegins == 0);
    dispose(u);
}
static std::string changedRelease(const std::function<void(JsonDocument &)> &change)
{
    JsonDocument doc;
    assert(!deserializeJson(doc, release));
    change(doc);
    std::string result;
    serializeJson(doc, result);
    return result;
}
static void expectState(const char *expected, const char *current = "V2.0.3",
                        const char *board = "wemos_d1_mini32")
{
    GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", current, board);
    u.begin([] { return true; });
    u.doCheck();
    const auto d = status(u);
    assert(d["state"] == expected);
    assert(ota.getStringCalls == 0 && ota.updateBegins == 0);
    if (std::strcmp(expected, "error") == 0)
    {
        assert(d["error"].is<const char *>());
        assert(!u.startUpdate());
    }
    dispose(u);
}
static void releaseVariants()
{
    for (const char *board :
         {"wemos_d1_mini32", "esp32c3_supermini", "esp32s3_supermini", "waveshare_esp32_s3_eth"})
    {
        reset();
        expectState("update_available", "V2.0.3", board);
    }
    reset();
    expectState("error", "V2.0.3", "m5stack_atom_lite"); // Absent from the captured release.
    for (const char *current : {"V2.0.0", "V2.0.1", "V2.0.2", "V2.0.3"})
    {
        reset();
        expectState("update_available", current);
    }
    for (const char *current : {"V2.0.4", "  v2.0.4  ", "2.0.4+build", "0.0.0-dev-2.0.6-RC1"})
    {
        reset();
        expectState("up_to_date", current);
    }
    for (const char *field :
         {"assets", "tag_name", "browser_download_url", "name", "draft", "prerelease"})
    {
        reset();
        const std::string f = field;
        ota.response = changedRelease(
            [&](JsonDocument &d)
            {
                if (f == "assets")
                    d.remove("assets");
                else if (f == "tag_name")
                {
                    d.remove("tag_name");
                    d.remove("name");
                }
                else if (f == "draft" || f == "prerelease")
                    d[field] = true;
                else
                    for (JsonObject a : d["assets"].as<JsonArray>())
                        a.remove(field);
            });
        expectState("error");
    }
    reset();
    ota.response = changedRelease([](JsonDocument &d) { d.remove("tag_name"); });
    expectState("update_available");
    reset();
    ota.response = changedRelease(
        [](JsonDocument &d)
        {
            for (JsonObject a : d["assets"].as<JsonArray>())
                a["name"] = std::string(a["name"].as<const char *>()) + ".bin";
        });
    expectState("error");
    reset();
    ota.response = changedRelease(
        [](JsonDocument &d)
        {
            for (JsonObject a : d["assets"].as<JsonArray>())
            {
                std::string n = a["name"].as<const char *>();
                const auto p = n.find("V2.0.4");
                if (p != std::string::npos)
                    n.replace(p, 6, "V2.0.3");
                a["name"] = n;
            }
        });
    expectState("error");
    // Expand an ignored field from the actual GitHub response, without changing
    // its firmware version, assets or expected update result.
    reset();
    ota.largeStringUnavailable = true;
    std::string expanded = "\"ignored\":[";
    for (int i = 0; i < 40; ++i)
    {
        if (i)
            expanded += ',';
        expanded += release;
    }
    expanded += "],";
    ota.response.insert(1, expanded);
    assert(ota.response.size() > 500000);
    expectState("update_available");
    std::puts("PASS: real release, all published board assets, repository versions, missing/wrong "
              "assets and large ignored fields.");
}
static void errors()
{
    for (const int code : {-1, 301, 403, 404, 429, 500, 503})
    {
        reset();
        ota.code = code;
        expectState("error");
        assert(ota.closed);
    }
    reset();
    ota.beginOk = false;
    expectState("error");
    assert(ota.getCalls == 0);
    reset();
    GitHubOtaUpdater offline("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
    offline.begin([] { return false; });
    offline.doCheck();
    assert(status(offline)["state"] == "error" && ota.getCalls == 0);
    dispose(offline);
    // Every cut of the recorded response is a genuinely incomplete response;
    // complete JSON followed only by original whitespace is valid.
    size_t complete = release.find_last_not_of(" \t\r\n") + 1;
    for (size_t cut = 0; cut < complete; ++cut)
    {
        reset();
        ota.response.resize(cut);
        expectState("error");
    }
    reset();
    ota.response = release;
    ota.response[0] = '!';
    expectState("error");
    for (size_t cut : {size_t(0), release.size() / 2, release.size() - 2})
    {
        reset();
        ota.failAt = cut;
        expectState("error");
    }
    for (const char *invalid :
         {"", "latest", "2.0", "2.0.4-", "2.0.4+", "02.0.4", "2.0.4-01", "2.0.4-a..b", "2.0.4++x"})
    {
        reset();
        ota.response = changedRelease(
            [&](JsonDocument &d)
            {
                d["tag_name"] = invalid;
                d.remove("name");
            });
        expectState("error");
    }
    std::printf("PASS: HTTP/network failures, all %u JSON truncations, read failures, invalid "
                "versions; no update writes.\n",
                unsigned(complete));
}
static void versionOrdering()
{
    // Tags and development marker come from this repository; diagnostic suffixes
    // are the compatibility examples in its original comparator. Prerelease
    // identifiers below follow the published SemVer precedence examples.
    const std::vector<const char *> order = {
        "V2.0.0", "V2.0.1", "V2.0.2",  "V2.0.3", "V2.0.4", "0.0.0-dev-2.0.6-RC1",
        "2.0.6",  "2.0.14", "2.0.14A", "2.0.14B"};
    for (size_t i = 0; i < order.size(); ++i)
        for (size_t j = 0; j < order.size(); ++j)
        {
            const int v = GitHubOtaUpdater::compareVersions(order[i], order[j]);
            assert(i == j ? v == 0 : (i < j ? v < 0 : v > 0));
        }
    const std::vector<const char *> semver = {"2.0.4-alpha", "2.0.4-alpha.1", "2.0.4-alpha.beta",
                                              "2.0.4-beta",  "2.0.4-beta.2",  "2.0.4-beta.11",
                                              "2.0.4-rc.1",  "2.0.4"};
    for (size_t i = 1; i < semver.size(); ++i)
        assert(GitHubOtaUpdater::compareVersions(semver[i - 1], semver[i]) < 0);
    assert(GitHubOtaUpdater::compareVersions("2.0.4+build.1", "2.0.4+build.2") == 0);
    const std::string huge(100, '9');
    const std::string major = huge + ".0.4";
    assert(GitHubOtaUpdater::compareVersions(major.c_str(), "2.0.4") > 0);
    assert(GitHubOtaUpdater::compareVersions("2.0.14a", "2.0.14A") == 0);
    std::puts("PASS: version ordering, development prefix, stable/prerelease, metadata, diagnostic "
              "suffixes and overflow-free numbers.");
}
static void taskLifecycle()
{
    reset();
    GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
    u.begin([] { return true; });
    assert(u.requestCheck());
    assert(u.isBusy());
    assert(!u.requestCheck() && !u.startUpdate() && ota.taskCount == 1);
    tasks[0]->fn(tasks[0]->param);
    assert(!u.isBusy() && u._task == nullptr && ota.deletes == 1);
    assert(status(u)["state"] == "update_available" && u.startUpdate());
    assert(!u.startUpdate() && !u.requestCheck() && ota.taskCount == 2);
    u.clearTask();
    u._restartPending = true;
    assert(!u.requestCheck() && !u.startUpdate());
    dispose(u);
    for (bool download : {false, true})
    {
        reset();
        GitHubOtaUpdater f("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        f.begin([] { return true; });
        if (download)
            f._state = GitHubOtaUpdater::State::UpdateAvailable;
        ota.taskFail = true;
        assert(!(download ? f.startUpdate() : f.requestCheck()));
        assert(status(f)["state"] == "error" && !f.isBusy() && f._task == nullptr);
        dispose(f);
    }
    reset();
    failNextSemaphoreCreate() = true;
    GitHubOtaUpdater noLock("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
    noLock.begin([] { return true; });
    assert(status(noLock)["state"] == "error" && !noLock.requestCheck() && !noLock.startUpdate());
    // Worker is allowed to finish inside xTaskCreate's return window.
    for (int i = 0; i < 20; ++i)
    {
        reset();
        ota.eager = true;
        GitHubOtaUpdater fast("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        fast.begin([] { return true; });
        assert(fast.requestCheck());
        finishTasks();
        assert(fast._task == nullptr && !fast.isBusy());
        assert(fast.requestCheck());
        finishTasks();
        dispose(fast);
    }
    reset();
    GitHubOtaUpdater concurrent("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
    concurrent.begin([] { return true; });
    std::vector<std::thread> requests;
    std::atomic<unsigned> accepted{0};
    for (int i = 0; i < 16; ++i)
        requests.emplace_back(
            [&]
            {
                if (concurrent.requestCheck())
                    ++accepted;
            });
    for (auto &t : requests)
    {
        t.join();
    }
    assert(accepted == 1 && ota.taskCount == 1);
    tasks[0]->fn(tasks[0]->param);
    dispose(concurrent);
    std::puts("PASS: duplicate/concurrent requests, immediate worker exit, task/mutex failure, "
              "retry and pending-restart guards.");
}
static void statusGuard()
{
    reset();
    GitHubOtaUpdater noLock("owner", "repo", "2.0.3", "board");
    { GitHubOtaUpdater::StatusGuard guard(noLock); assert(!guard); }

    GitHubOtaUpdater updater("owner", "repo", "2.0.3", "board");
    updater.begin([] { return true; });
    for (auto state : {GitHubOtaUpdater::State::Idle, GitHubOtaUpdater::State::UpToDate,
                       GitHubOtaUpdater::State::UpdateAvailable, GitHubOtaUpdater::State::Error})
    {
        updater._state = state;
        GitHubOtaUpdater::StatusGuard guard(updater);
        assert(guard);
    }
    for (int reason = 0; reason < 4; ++reason)
    {
        updater._state = reason == 0 ? GitHubOtaUpdater::State::Checking :
                         reason == 1 ? GitHubOtaUpdater::State::Downloading : GitHubOtaUpdater::State::Idle;
        updater._task = reason == 2 ? reinterpret_cast<void *>(1) : nullptr;
        updater._restartPending = reason == 3;
        GitHubOtaUpdater::StatusGuard guard(updater);
        assert(!guard);
    }
    updater._state = GitHubOtaUpdater::State::Idle;
    updater._task = nullptr;
    updater._restartPending = false;

    auto *mutex = static_cast<SemaphoreHandle_t>(updater._lockHandle);
    mutex->lock();
    std::mutex sync;
    std::condition_variable cv;
    bool done = false, acquired = true;
    std::thread reader([&] {
        GitHubOtaUpdater::StatusGuard guard(updater);
        { std::lock_guard<std::mutex> lock(sync); acquired = static_cast<bool>(guard); done = true; }
        cv.notify_one();
    });
    bool returnedWhileLocked;
    {
        std::unique_lock<std::mutex> lock(sync);
        returnedWhileLocked = cv.wait_for(lock, std::chrono::milliseconds(250), [&] { return done; });
    }
    mutex->unlock();
    reader.join();
    assert(returnedWhileLocked && !acquired);

    // A status allocation already in progress owns the same lock used when
    // registering the worker; startup can proceed only after guard destruction.
    bool started = false, requestDone = false, accepted = false;
    std::thread request;
    {
        GitHubOtaUpdater::StatusGuard guard(updater);
        assert(guard);
        request = std::thread([&] {
            { std::lock_guard<std::mutex> lock(sync); started = true; }
            cv.notify_one();
            const bool result = updater.requestCheck();
            { std::lock_guard<std::mutex> lock(sync); accepted = result; requestDone = true; }
            cv.notify_one();
        });
        std::unique_lock<std::mutex> lock(sync);
        assert(cv.wait_for(lock, std::chrono::seconds(2), [&] { return started; }));
        assert(!cv.wait_for(lock, std::chrono::milliseconds(100), [&] { return requestDone; }));
        assert(ota.taskCount == 0);
    }
    request.join();
    assert(accepted && requestDone && ota.taskCount == 1);
    { GitHubOtaUpdater::StatusGuard guard(updater); assert(!guard); }
    tasks[0]->fn(tasks[0]->param);
    { GitHubOtaUpdater::StatusGuard guard(updater); assert(guard); }
    dispose(updater);
    std::puts("PASS: status guard is nonblocking, rejects busy states and serializes worker startup.");
}

static void packetGaps()
{
    unsigned cases = 0;
    for (size_t at = 0; at < release.size(); at += 257)
    {
        reset();
        ota.gapAt = at;
        ota.gapDuration = 1200;
        GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        u.begin([] { return true; });
        u.doCheck();
        assert(status(u)["state"] == "update_available");
        assert(ota.gapStarted && ota.gapFinished && ota.getStringCalls == 0);
        assert(ota.updateBegins == 0);
        dispose(u);
        ++cases;
    }
    for (const uint32_t start : {100U, 0xfffffff0U})
        for (const uint32_t gap : {14999U, 15000U, 15001U, 16000U, 0xffffffffU})
        {
            reset();
            testNow = start;
            ota.gapAt = release.find("tag_name");
            ota.gapDuration = gap;
            GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
            u.begin([] { return true; });
            u.doCheck();
            const auto first = status(u);
            if (gap <= 15000)
                assert(first["state"] == "update_available");
            else
            {
                assert(first["state"] == "error" && !first["busy"].as<bool>());
                assert(!u.startUpdate() && ota.updateBegins == 0);
                assert(static_cast<uint32_t>(testNow - start) == 15000);
                ota.closed = false;
                ota.offset = 0;
                ota.gapAt = SIZE_MAX;
                u.doCheck();
                assert(status(u)["state"] == "update_available");
            }
            assert(ota.closed && ota.updateBegins == 0);
            dispose(u);
        }
    std::printf("PASS: %u real-release packet boundaries, bounded pauses, timeout/retry and timer wrap.\n", cases);
}

static void transportRetry()
{
    for (const int code : {-1, -5, -11})
    {
        reset();
        ota.httpCodes = {code, 200};
        GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        u.begin([] { return true; });
        u.doCheck();
        assert(status(u)["state"] == "update_available");
        assert(ota.getCalls == 2 && ota.clientStops == 1 && testNow == 1100);
        assert(ota.updateBegins == 0);
        dispose(u);
        reset();
        ota.code = code;
        GitHubOtaUpdater failed("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        failed.begin([] { return true; });
        failed.doCheck();
        assert(status(failed)["state"] == "error");
        assert(ota.getCalls == 2 && ota.clientStops == 1 && testNow == 1100);
        assert(!failed.startUpdate() && ota.updateBegins == 0);
        dispose(failed);
    }
    for (const int code : {403, 404, 429, 500, 503})
    {
        reset();
        ota.httpCodes = {code, 200};
        GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        u.begin([] { return true; });
        u.doCheck();
        assert(status(u)["state"] == "error");
        assert(ota.getCalls == 1 && ota.clientStops == 0 && testNow == 100);
        dispose(u);
    }
    std::puts("PASS: one bounded metadata transport reconnect; permanent failures and HTTP rejection stop safely.");
}

static void allocationFailures()
{
#ifdef TEST_WRAP_ALLOC
    unsigned errors = 0;
    for (size_t i = 0; i < 64; ++i)
    {
        reset();
        GitHubOtaUpdater u("softwarecrash", "Solar2MQTT", "V2.0.3", "wemos_d1_mini32");
        u.begin([] { return true; });
        allocationIndex = 0;
        allocationSizeCount = 0;
        failAllocation = i;
        allocationFault = true;
        u.doCheck();
        allocationFault = false;
        const auto d = status(u);
        if (d["state"] == "error")
        {
            ++errors;
            const char *error = d["error"];
            assert(error && (std::strstr(error, "memory") || std::strstr(error, "NoMemory")));
            assert(!u.startUpdate() && ota.updateBegins == 0);
        }
        else
            assert(d["state"] == "update_available");
        assert(ota.closed);
        dispose(u);
    }
    assert(errors >= 2);
    std::printf(
        "PASS: %u actual ArduinoJson allocation failures return an error without update writes.\n",
        errors);
#endif
}
int main(int argc, char **argv)
{
    assert(argc >= 2);
    std::ifstream input(argv[1], std::ios::binary);
    assert(input);
    release.assign(std::istreambuf_iterator<char>(input), {});
    if (argc >= 3 && std::strcmp(argv[2], "versions") == 0)
        versions();
    else if (argc >= 3 && std::strcmp(argv[2], "stream") == 0)
        actualRelease(true);
    else
    {
        actualRelease(true);
        packetGaps();
        transportRetry();
        versions();
        versionOrdering();
        releaseVariants();
        errors();
        taskLifecycle();
        statusGuard();
        allocationFailures();
    }
    std::puts("GitHub OTA production checks passed.");
}
