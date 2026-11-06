#include <Arduino.h>
#include <ArduinoJson.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <freertos/semphr.h>
#include "core/Ds18b20Service.h"
#include "core/InternalTemperatureService.h"
#include "core/LogSerial.h"
#include "descriptors.h"

uint32_t testNow = 100;
FakeEsp ESP;
FakeLogSerial LogSerial;
bool sensorFresh = true;
bool sensorHasTemperature = true;
float sensorTemperature = 21.5f;
float internalTemperature = 35.0f;
float temperatureRead() { return internalTemperature; }

struct TrackingAllocator : ArduinoJson::Allocator
{
    size_t calls = 0;
    size_t failAt = SIZE_MAX;
    std::map<void *, size_t> blocks;

    void *allocate(size_t n) override
    {
        if (calls++ == failAt)
            return nullptr;
        void *p = malloc(n);
        if (p)
            blocks[p] = n;
        return p;
    }

    void deallocate(void *p) override
    {
        if (p)
        {
            assert(blocks.count(p));
            blocks.erase(p);
            free(p);
        }
    }

    void *reallocate(void *p, size_t n) override
    {
        if (calls++ == failAt)
            return nullptr;
        const uintptr_t address = reinterpret_cast<uintptr_t>(p);
        const size_t old = p ? blocks.at(p) : 0;
        if (p)
            blocks.erase(p);
        void *q = realloc(p, n);
        if (q)
            blocks[q] = n;
        else if (address)
            blocks[reinterpret_cast<void *>(address)] = old;
        return q;
    }
} allocator;

using RealJsonDocument = ArduinoJson::JsonDocument;
class TracedJsonDocument : public RealJsonDocument
{
public:
    explicit TracedJsonDocument(ArduinoJson::Allocator *a = &::allocator) : RealJsonDocument(a) {}
};

#define JsonDocument TracedJsonDocument
#define private public
#include "core/SolarState.h"
#undef private

JsonDocument g_stateDoc;
JsonObject deviceJson, staticData, liveData;
#include "production-snapshot.inc"

static void loadFixture(const char *path, RealJsonDocument &doc)
{
    std::ifstream file(path);
    std::string contents((std::istreambuf_iterator<char>(file)), {});
    assert(!deserializeJson(doc, contents));
}

static std::string json(JsonVariantConst value)
{
    std::string output;
    serializeJson(value, output);
    return output;
}

static void loadState(const RealJsonDocument &fixture)
{
    g_stateDoc.clear();
    assert(g_stateDoc.set(fixture["data"]));
    // Existing test_restart.cpp software-counter sentinels, not inverter measurements.
    g_stateDoc["LiveData"]["Battery_Charge_Energy"] = 12.125;
    g_stateDoc["LiveData"]["Battery_Discharge_Energy"] = 23.25;
    assert(!g_stateDoc.overflowed());
}

struct Snapshot
{
    std::string raw, device, live, esp, status;
};

static Snapshot capture(SolarState &state)
{
    JsonDocument out;
    assert(state.snapshotTo(out));
    assert(!out.overflowed());
    return {json(out["RawData"]), json(out["DeviceData"]), json(out["LiveData"]),
            json(out["EspData"]), json(out["Status"])};
}

static bool equal(const Snapshot &a, const Snapshot &b)
{
    return a.raw == b.raw && a.device == b.device && a.live == b.live &&
           a.esp == b.esp && a.status == b.status;
}

// A no-op local commit must not allocate; a changed field must still be
// committed transactionally and leave the published snapshot intact on failure.
static void localCommitFastPath(const RealJsonDocument &fixture, const RealJsonDocument &alternate)
{
    SolarState state;
    state._snapshotMutex = xSemaphoreCreateMutex();
    loadState(fixture);
    assert(state.commitInverterState());
    assert(state.commitLocalState());
    const Snapshot before = capture(state);

    const auto expectNoAllocation = [&]()
    {
        const size_t start = allocator.calls;
        allocator.failAt = start;
        const bool committed = state.commitLocalState();
        const size_t end = allocator.calls;
        allocator.failAt = SIZE_MAX;
        assert(committed && end == start);
        assert(equal(capture(state), before));
    };
    expectNoAllocation();

    // A changed inverter-only value is not part of the local snapshot.
    g_stateDoc["LiveData"]["PV2_Input_Voltage"] =
        alternate["data"]["LiveData"]["PV2_Input_Voltage"];
    expectNoAllocation();

    for (int change = 0; change < 10; ++change)
    {
        loadState(fixture);
        switch (change)
        {
        case 0:
            g_stateDoc["EspData"]["runtime"] = alternate["data"]["EspData"]["runtime"];
            break;
        case 1:
            g_stateDoc["EspData"].as<JsonObject>().remove("runtime");
            break;
        case 2:
            g_stateDoc["Status"]["protocol"] = alternate["data"]["Status"]["protocol"];
            break;
        case 3:
            g_stateDoc["Status"].as<JsonObject>().remove("protocol");
            break;
        case 4:
            g_stateDoc["RawData"]["CommandAnswer"] = alternate["data"]["RawData"]["QPIGS2"];
            break;
        case 5:
            g_stateDoc["RawData"].as<JsonObject>().remove("CommandAnswer");
            break;
        case 6:
            g_stateDoc["LiveData"]["Battery_Charge_Energy"] = 23.25;
            break;
        case 7:
            g_stateDoc["LiveData"].as<JsonObject>().remove("Battery_Charge_Energy");
            break;
        case 8:
            g_stateDoc["LiveData"]["Battery_Discharge_Energy"] = 12.125;
            break;
        case 9:
            g_stateDoc["LiveData"].as<JsonObject>().remove("Battery_Discharge_Energy");
            break;
        }
        assert(!g_stateDoc.overflowed());
        const size_t start = allocator.calls;
        allocator.failAt = start;
        const bool committed = state.commitLocalState();
        const size_t end = allocator.calls;
        allocator.failAt = SIZE_MAX;
        assert(!committed && end > start);
        assert(equal(capture(state), before));
    }
    // Adding a local key must also invalidate the equality fast path.
    loadState(fixture);
    g_stateDoc["LiveData"].as<JsonObject>().remove("Battery_Charge_Energy");
    assert(state.commitLocalState());
    const Snapshot withoutCharge = capture(state);
    g_stateDoc["LiveData"]["Battery_Charge_Energy"] = 12.125;
    const size_t start = allocator.calls;
    allocator.failAt = start;
    const bool committed = state.commitLocalState();
    const size_t end = allocator.calls;
    allocator.failAt = SIZE_MAX;
    assert(!committed && end > start);
    assert(equal(capture(state), withoutCharge));

    // Equal counts and null values must not hide a change in local key identity.
    loadState(fixture);
    JsonObject live = g_stateDoc["LiveData"].as<JsonObject>();
    live.remove("Battery_Charge_Energy");
    live.remove("Battery_Discharge_Energy");
    live["Battery_Charge_Energy"] = nullptr;
    assert(!live["Battery_Charge_Energy"].isUnbound());
    assert(state.commitLocalState());
    const Snapshot withNullCharge = capture(state);
    const size_t liveCount = live.size();
    live.remove("Battery_Charge_Energy");
    live["Battery_Discharge_Energy"] = nullptr;
    assert(live.size() == liveCount);
    assert(!live["Battery_Discharge_Energy"].isUnbound());
    const size_t nullStart = allocator.calls;
    allocator.failAt = nullStart;
    const bool nullCommitted = state.commitLocalState();
    const size_t nullEnd = allocator.calls;
    allocator.failAt = SIZE_MAX;
    assert(!nullCommitted && nullEnd > nullStart);
    assert(equal(capture(state), withNullCharge));
    vSemaphoreDelete(state._snapshotMutex);
}

// The real services keep a JsonObject member. Recovery swaps and frees the
// previous working document, so every loop must refresh that member first.
static void sensorBindingsAfterRecovery(const RealJsonDocument &fixture)
{
    SolarState state;
    state._snapshotMutex = xSemaphoreCreateMutex();
    loadState(fixture);
    state.refreshBindings();
    assert(state.commitInverterState());
    assert(state.commitLocalState());

    Ds18b20Service ds;
    InternalTemperatureService internal;
    unsigned dsCallbacks = 0, internalCallbacks = 0;
    ds.setCallback([&](uint8_t, float) { ++dsCallbacks; });
    internal.setCallback([&]() { ++internalCallbacks; });
    ds.begin(4, liveData);
    internal.begin(g_stateDoc["EspData"].as<JsonObject>());
    testNow += 1000;
    ds.loop(liveData);
    assert(ds.sensorCount() == 1 && dsCallbacks == 1);
    assert(g_stateDoc["LiveData"]["DS18B20_1"].as<float>() == 21.5f);
    assert(g_stateDoc["EspData"][DESCR_ESP_Internal_Temperature].as<float>() == 35.0f);
    assert(state.commitLocalState());

    // Force the actual producer overflow path without clearing the document:
    // recoverWorkingState() itself then swaps and destroys the old storage.
    const std::string failedKey(512, 'x');
    allocator.failAt = allocator.calls;
    g_stateDoc["RawData"][failedKey] = true;
    allocator.failAt = SIZE_MAX;
    assert(g_stateDoc.overflowed());
    assert(state.recoverWorkingState());
    assert(!g_stateDoc.overflowed());

    sensorTemperature = 27.5f;
    internalTemperature = 34.5f;
    testNow += 10000;
    ds.loop(liveData);
    internal.loop(state.doc()["EspData"].as<JsonObject>());
    assert(g_stateDoc["LiveData"]["DS18B20_1"].as<float>() == 27.5f);
    assert(g_stateDoc["EspData"][DESCR_ESP_Internal_Temperature].as<float>() == 34.5f);
    assert(dsCallbacks == 2 && internalCallbacks == 1);
    assert(state.commitLocalState());
    JsonDocument published;
    assert(state.snapshotTo(published));
    assert(published["LiveData"]["DS18B20_1"].as<float>() == 27.5f);
    assert(published["EspData"][DESCR_ESP_Internal_Temperature].as<float>() == 34.5f);

    sensorHasTemperature = false;
    sensorFresh = false;
    internalTemperature = NAN;
    testNow += 10000;
    ds.loop(liveData);
    internal.loop(state.doc()["EspData"].as<JsonObject>());
    assert(g_stateDoc["LiveData"]["DS18B20_1"].isNull());
    assert(g_stateDoc["EspData"][DESCR_ESP_Internal_Temperature].isNull());
    assert(internalCallbacks == 2);
    assert(state.commitLocalState());
    assert(state.snapshotTo(published));
    assert(published["LiveData"]["DS18B20_1"].isNull());
    assert(published["EspData"][DESCR_ESP_Internal_Temperature].isNull());

    sensorFresh = true;
    sensorHasTemperature = true;
    sensorTemperature = 21.5f;
    internalTemperature = 35.0f;
    testNow += 10000;
    ds.loop(liveData);
    internal.loop(state.doc()["EspData"].as<JsonObject>());
    assert(g_stateDoc["LiveData"]["DS18B20_1"].as<float>() == 21.5f);
    assert(g_stateDoc["EspData"][DESCR_ESP_Internal_Temperature].as<float>() == 35.0f);
    assert(dsCallbacks == 3 && internalCallbacks == 3);
    std::puts("PASS: both production sensor services rebind after recovery and publish/clear/recover values.");
    vSemaphoreDelete(state._snapshotMutex);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    RealJsonDocument gk, max;
    loadFixture(argv[1], gk);
    loadFixture(argv[2], max);
    sensorBindingsAfterRecovery(gk);
    localCommitFastPath(gk, max);
    localCommitFastPath(max, gk);

    // Missing mutex means no unprotected publication, read or recovery.
    {
        SolarState uninitialized;
        loadState(gk);
        JsonDocument output;
        output["stale"] = true;
        assert(!uninitialized.commitInverterState());
        assert(!uninitialized.commitLocalState());
        assert(!uninitialized.snapshotTo(output));
        assert(output.isNull() || output.size() == 0);
        assert(!uninitialized.recoverWorkingState());
    }

    size_t commitCases = 0, sourceCases = 0, localCases = 0, readCases = 0, recoveryCases = 0;
    for (int reverse = 0; reverse < 2; ++reverse)
    {
        const auto &old = reverse ? max : gk;
        const auto &next = reverse ? gk : max;
        Snapshot expected;
        {
            SolarState control;
            control._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(control.commitInverterState());
            assert(control.commitLocalState());
            loadState(next);
            assert(control.commitInverterState());
            expected = capture(control);
            vSemaphoreDelete(control._snapshotMutex);
        }

        for (size_t failure = 0; failure < 500; ++failure)
        {
            SolarState state;
            state._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(state.commitInverterState());
            assert(state.commitLocalState());
            const Snapshot before = capture(state);
            loadState(next);
            const size_t start = allocator.calls;
            allocator.failAt = start + failure;
            const bool committed = state.commitInverterState();
            const bool injected = allocator.calls > start + failure;
            allocator.failAt = SIZE_MAX;
            if (!injected)
            {
                assert(committed);
                assert(equal(capture(state), expected));
                vSemaphoreDelete(state._snapshotMutex);
                break;
            }
            const Snapshot after = capture(state);
            if (committed ? !equal(after, expected) : !equal(after, before))
            {
                std::cerr << "FAIL producer atomicity reverse=" << reverse << " fault=" << failure << "\n";
                return 1;
            }
            assert(state.commitInverterState());
            assert(equal(capture(state), expected));
            vSemaphoreDelete(state._snapshotMutex);
            ++commitCases;
        }

        for (size_t failure = 0; failure < 500; ++failure)
        {
            SolarState state;
            state._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(state.commitInverterState());
            assert(state.commitLocalState());
            const Snapshot before = capture(state);
            g_stateDoc.clear();
            const size_t start = allocator.calls;
            allocator.failAt = start + failure;
            g_stateDoc.set(next["data"]);
            const bool injected = allocator.calls > start + failure;
            allocator.failAt = SIZE_MAX;
            if (!injected)
            {
                vSemaphoreDelete(state._snapshotMutex);
                break;
            }
            assert(g_stateDoc.overflowed());
            assert(!state.commitInverterState());
            assert(!state.commitLocalState());
            assert(equal(capture(state), before));
            assert(state.recoverWorkingState());
            assert(!g_stateDoc.overflowed());
            assert(equal(capture(state), before));
            assert(g_stateDoc["LiveData"]["Battery_Charge_Energy"].as<double>() == 12.125);
            loadState(next);
            assert(state.commitInverterState());
            assert(equal(capture(state), expected));
            vSemaphoreDelete(state._snapshotMutex);
            ++sourceCases;
        }

        for (size_t failure = 0; failure < 500; ++failure)
        {
            SolarState state;
            state._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(state.commitInverterState());
            assert(state.commitLocalState());
            const Snapshot before = capture(state);
            g_stateDoc.clear();
            allocator.failAt = allocator.calls;
            g_stateDoc.set(next["data"]);
            allocator.failAt = SIZE_MAX;
            assert(g_stateDoc.overflowed());
            const size_t start = allocator.calls;
            allocator.failAt = start + failure;
            const bool recovered = state.recoverWorkingState();
            const bool injected = allocator.calls > start + failure;
            allocator.failAt = SIZE_MAX;
            if (!injected)
            {
                assert(recovered);
                vSemaphoreDelete(state._snapshotMutex);
                break;
            }
            if (!recovered)
            {
                assert(g_stateDoc.overflowed());
                assert(equal(capture(state), before));
                assert(state.recoverWorkingState());
            }
            assert(!g_stateDoc.overflowed());
            assert(equal(capture(state), before));
            vSemaphoreDelete(state._snapshotMutex);
            ++recoveryCases;
        }

        for (size_t failure = 0; failure < 500; ++failure)
        {
            SolarState state;
            state._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(state.commitInverterState());
            assert(state.commitLocalState());
            const Snapshot before = capture(state);
            // Reuse the existing local counter sentinel values, swapped to distinguish updates.
            g_stateDoc["LiveData"]["Battery_Charge_Energy"] = 23.25;
            g_stateDoc["LiveData"]["Battery_Discharge_Energy"] = 12.125;
            const size_t start = allocator.calls;
            allocator.failAt = start + failure;
            const bool committed = state.commitLocalState();
            const bool injected = allocator.calls > start + failure;
            allocator.failAt = SIZE_MAX;
            if (!injected)
            {
                assert(committed);
                vSemaphoreDelete(state._snapshotMutex);
                break;
            }
            if (!committed)
                assert(equal(capture(state), before));
            assert(state.commitLocalState());
            JsonDocument out;
            assert(state.snapshotTo(out));
            assert(out["LiveData"]["Battery_Charge_Energy"].as<double>() == 23.25 &&
                   out["LiveData"]["Battery_Discharge_Energy"].as<double>() == 12.125);
            assert(json(out["RawData"]) == before.raw && json(out["DeviceData"]) == before.device);
            vSemaphoreDelete(state._snapshotMutex);
            ++localCases;
        }

        for (size_t failure = 0; failure < 500; ++failure)
        {
            SolarState state;
            state._snapshotMutex = xSemaphoreCreateMutex();
            loadState(old);
            assert(state.commitInverterState());
            assert(state.commitLocalState());
            const Snapshot before = capture(state);
            JsonDocument out;
            out["stale"] = true;
            const size_t start = allocator.calls;
            allocator.failAt = start + failure;
            const bool ok = state.snapshotTo(out);
            const bool injected = allocator.calls > start + failure;
            allocator.failAt = SIZE_MAX;
            if (!injected)
            {
                assert(ok);
                vSemaphoreDelete(state._snapshotMutex);
                break;
            }
            if (!ok)
                assert(out.isNull() || out.size() == 0);
            assert(equal(capture(state), before));
            vSemaphoreDelete(state._snapshotMutex);
            ++readCases;
        }
    }
    assert(commitCases > 0 && sourceCases > 0 && localCases > 0 && readCases > 0 && recoveryCases > 0);
    std::cout << "PASS snapshot faults producer=" << commitCases << " source=" << sourceCases
              << " local=" << localCases << " reader=" << readCases << " recovery=" << recoveryCases
              << " pointerBytes=" << sizeof(void *) << "\n";
}
