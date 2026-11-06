#include <Arduino.h>
#include "BatteryStorageMock.h"
#include <cassert>
#include <cmath>
#include <vector>
#include <map>
#include <set>
#include "descriptors.h"
#include "solar/InverterHardware.h"
#define private public
#include "modbus/modbus.h"
#include "core/SolarState.h"
#include "core/EnergyBacklog.h"
#undef private

namespace battery_test
{
Storage storage;
}
FakeLittleFS LittleFS;
uint32_t testNow = 100, pv2DelayMs = 0;
FakeEsp ESP;
TestWire wire;
TestHardware g_inverterHardwareConfig;
void writeLog(const char *, ...) {}
void MODBUS::stepPowmrHunt() {}
void MODBUS::capturePowmrSocSample() {}
static void ignoredNonBatteryCallback(JsonObject *, uint16_t *, const modbus_register_t *,
                                      MODBUS_COM &)
{
}
#include "modbus_profiles_generated.h"
#include "modbus_timing_generated.h"

static void near(double actual, double expected)
{
    if (!std::isfinite(actual) || !std::isfinite(expected) || std::fabs(actual - expected) > 1e-7)
    {
        std::fprintf(stderr, "actual %.12f expected %.12f\n", actual, expected);
        assert(false);
    }
}
struct Profile
{
    const char *name;
    const modbus_register_t *regs;
    size_t count;
    protocol_type_t protocol;
    bool supported;
};
#define PROFILE(name, protocol, supported)                                                         \
    {#name, profile_##name::live, sizeof(profile_##name::live) / sizeof(modbus_register_t),        \
     protocol, supported}
static const Profile profiles[] = {
    PROFILE(smg, MODBUS_SMG, true),           PROFILE(smg_ii_11kw, MODBUS_SMG_II_11KW, true),
    PROFILE(must_pv_ph18, MODBUS_MUST, true), PROFILE(powmr, MODBUS_POWMR, true),
    PROFILE(anenji, MODBUS_ANENJI, false),    PROFILE(anenji_srne, MODBUS_ANENJI_SRNE, false),
    PROFILE(deye, MODBUS_DEYE, false)};

static uint16_t encode(const modbus_register_t &reg, double value)
{
    const bool tenth = reg.type == REGISTER_TYPE_DIEMATIC_ONE_DECIMAL ||
                       reg.type == REGISTER_TYPE_INT16_ONE_DECIMAL ||
                       reg.type == REGISTER_TYPE_U16_SWAP_ONE_DECIMAL;
    const bool hundredth = reg.type == REGISTER_TYPE_DIEMATIC_TWO_DECIMAL ||
                           reg.type == REGISTER_TYPE_INT16_TWO_DECIMAL;
    const long n = std::lround((value - reg.offset) * (hundredth ? 100 : tenth ? 10 : 1));
    uint16_t raw = static_cast<uint16_t>(n);
    if ((reg.type == REGISTER_TYPE_DIEMATIC_ONE_DECIMAL ||
         reg.type == REGISTER_TYPE_DIEMATIC_TWO_DECIMAL) &&
        n < 0)
        raw = static_cast<uint16_t>(-n) | 0x8000;
    if (reg.type == REGISTER_TYPE_U16_SWAP || reg.type == REGISTER_TYPE_U16_SWAP_ONE_DECIMAL)
        raw = uint16_t(raw >> 8 | raw << 8);
    return raw;
}

struct Fixture
{
    HardwareSerial serial;
    SolarState state;
    EnergyBacklog energy;
    MODBUS driver;
    const Profile &profile;
    unsigned completions = 0;
    uint32_t step;
    Fixture(const Profile &p, uint32_t interval, uint32_t start = 100)
        : driver(&serial, 1, 2), profile(p), step(std::max(interval, 200U))
    {
        battery_test::storage = battery_test::Storage{};
        wire = TestWire{};
        testNow = start;
        pv2DelayMs = 0;
        state.begin();
        energy.begin();
        deviceJson = state.doc()["DeviceData"].to<JsonObject>();
        staticData = state.doc()["StaticData"].to<JsonObject>();
        liveData = state.doc()["LiveData"].to<JsonObject>();
        driver.device =
            p.protocol == MODBUS_POWMR ? static_cast<ModbusDevice *>(new PowMr) : new ModbusDevice;
        driver.device->live = p.regs;
        driver.device->liveCount = p.count;
        driver.device->protocol = p.protocol;
        driver.prepareRegisters();
        driver.requestStaticData = false;
        driver.setPollIntervalMs(interval);
        driver.callback([this]() { publish(); });
        setPower(500);
    }
    ~Fixture()
    {
        vSemaphoreDelete(state._snapshotMutex);
        state._snapshotMutex = nullptr;
    }
    void setPower(double watts)
    {
        for (size_t i = 0; i < profile.count; i++)
        {
            const auto &r = profile.regs[i];
            double value = 0;
            if (std::strcmp(r.name, DESCR_Battery_Voltage) == 0)
                value = 50;
            if (std::strcmp(r.name, DESCR_Battery_Load) == 0)
                value = watts / 50;
            if (std::strcmp(r.name, DESCR_Battery_Charge_Current) == 0)
                value = std::max(0.0, watts / 50);
            if (std::strcmp(r.name, DESCR_Battery_Discharge_Current) == 0)
                value = std::max(0.0, -watts / 50);
            wire.words[r.id] = encode(r, value);
        }
    }
    void publish()
    {
        driver.acceptBatterySample();
        state.commitInverterState();
        ++completions;
    }
    void tick()
    {
        testNow += step;
        driver.loop();
        if (driver.consumePowMrLivePassCompleted())
            publish();
        energy.updateBatteryEnergy(state, driver.batterySampleTiming(), testNow);
    }
    void pass()
    {
        const auto before = completions;
        for (size_t i = 0; i < profile.count + 2 && completions == before; i++)
            tick();
        assert(completions == before + 1);
    }
    void idle(uint32_t ms)
    {
        testNow += ms;
        energy.updateBatteryEnergy(state, driver.batterySampleTiming(), testNow);
    }
};

static void slowCycles()
{
    // Independent oracle: 500 W for ten 26-second intervals = 36.111... Wh.
    Fixture smg(profiles[0], 1000);
    assert(smg.profile.count == 26);
    smg.pass();
    const auto first = testNow;
    for (unsigned i = 0; i < 10; i++)
        smg.pass();
    assert(testNow - first == 260000);
    near(smg.energy._batteryChargeWh, 500 * 260.0 / 3600);
    near(smg.energy._batteryDischargeWh, 0);
    for (const auto &p : profiles)
        for (uint32_t interval : {25U, 100U, 1000U, 5000U})
        {
            Fixture f(p, interval);
            f.pass();
            const auto sample = f.driver.batterySampleTiming();
            const auto at = testNow;
            if (!p.supported)
            {
                assert(!sample.available);
                f.pass();
                near(f.energy._batteryChargeWh, 0);
                continue;
            }
            assert(sample.available && sample.sequence == 1);
            assert(sample.validForMs <= kMaxBatterySampleValidityMs);
            for (unsigned i = 0; i < 3; i++)
                f.pass();
            near(f.energy._batteryChargeWh, 500.0 * uint32_t(testNow - at) / 3600000);
            near(f.energy._batteryDischargeWh, 0);
            if (interval == 5000 && p.count >= 24)
                assert(sample.validForMs > 120000);
        }
    std::puts("PASS: all seven production Modbus profiles at 25/100/1000/5000 ms; exact 500 W "
              "integration across slow cycles.");
}

static void failureFreshness()
{
    Fixture f(profiles[0], 1000);
    f.pass();
    const auto sample = f.driver.batterySampleTiming();
    assert(sample.available);
    wire.words[232] = 0xffff; // voltage remains valid; current decode fails
    f.pass();
    assert(f.driver.batterySampleTiming().sequence == sample.sequence);
    assert(f.driver.connection);
    f.pass();
    f.pass();
    const double expired = f.energy._batteryChargeWh;
    f.pass();
    near(f.energy._batteryChargeWh, expired);
    assert(f.driver.batterySampleTiming().sampledAtMs == sample.sampledAtMs);
    assert(f.driver.batterySampleTiming().validForMs == sample.validForMs);
    f.setPower(250);
    f.pass();
    assert(f.driver.batterySampleTiming().sequence == sample.sequence + 1);
    const auto resume = testNow;
    f.pass();
    near(f.energy._batteryChargeWh, expired + 250.0 * (testNow - resume) / 3600000);
    // A failed unrelated block does not invalidate freshly read battery inputs.
    wire.failures.insert(100);
    const auto seq = f.driver.batterySampleTiming().sequence;
    f.pass();
    assert(f.driver.batterySampleTiming().sequence == seq + 1);
    // A partial battery-block timeout must not reuse a successful status read.
    wire.failures.clear();
    wire.failures.insert(215);
    const auto stable = f.driver.batterySampleTiming();
    f.pass();
    assert(f.driver.batterySampleTiming().sequence == stable.sequence);
    assert(f.driver.batterySampleTiming().acceptedAtMs == stable.acceptedAtMs);
    // Static-only callbacks neither renew an old reading nor create a new one.
    static const modbus_register_t stats[] = {
        {999, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Battery_Voltage}};
    f.driver.static_info = {&staticData, stats, 1, 0};
    f.driver.requestStaticData = true;
    f.tick();
    assert(f.driver.batterySampleTiming().sequence == stable.sequence);
    Fixture fresh(profiles[0], 100);
    fresh.driver.static_info = {&staticData, stats, 1, 0};
    fresh.driver.requestStaticData = true;
    fresh.tick();
    assert(!fresh.driver.batterySampleTiming().available);
    std::puts("PASS: invalid decoded current, battery/nonbattery read failures, frozen expiry, "
              "recovery and static-only callbacks.");
}

static void timestampsAndBounds()
{
    Fixture f(profiles[0], 1000);
    wire.latencyMs = 17;
    f.pass();
    const auto s = f.driver.batterySampleTiming();
    uint32_t acquisition = 0;
    for (const auto &r : wire.reads)
        if (r.start == 201)
            acquisition = r.at;
    assert(s.sampledAtMs == acquisition && uint32_t(s.acceptedAtMs - s.sampledAtMs) > 15000);
    assert(s.acceptedAtMs == testNow); // cache acquisition, not delayed register decode
    Fixture pow(profiles[3], 1000);
    pv2DelayMs = 4300;
    pow.pass();
    const auto p = pow.driver.batterySampleTiming();
    assert(p.acceptedAtMs == testNow && uint32_t(p.acceptedAtMs - p.sampledAtMs) > 15000);
    Fixture wrap(profiles[0], 1000, 0xfffff000UL);
    wrap.pass();
    const auto start = testNow;
    wrap.pass();
    near(wrap.energy._batteryChargeWh, 500.0 * uint32_t(testNow - start) / 3600000);
    Fixture capped(profiles[0], 0xffffffffUL);
    capped.step = 200;
    capped.pass();
    assert(capped.driver.batterySampleTiming().validForMs == kMaxBatterySampleValidityMs);
    Fixture stop(profiles[0], 1000);
    stop.pass();
    const auto seq = stop.driver.batterySampleTiming().sequence;
    stop.driver._powmrDumpRunning = true;
    assert(!stop.driver.batterySampleTiming().available);
    stop.tick();
    assert(stop.driver.batterySampleTiming().sequence == seq);
    stop.driver._powmrDumpRunning = false;
    stop.driver.connection = false;
    assert(!stop.driver.batterySampleTiming().available);
    std::puts("PASS: cache acquisition time, delayed PowMr publication, 32-bit wrap, bounded "
              "leases and diagnostic/disconnection pause.");
}

static void zeroAndDirections()
{
    for (size_t index : {size_t(0), size_t(1), size_t(2), size_t(3)})
    {
        Fixture f(profiles[index], 1000);
        f.setPower(0);
        f.pass();
        assert(f.driver.batterySampleTiming().available);
        f.pass();
        near(f.energy._batteryChargeWh, 0);
        f.setPower(500);
        f.pass();
        const auto at = testNow;
        f.pass();
        const double charged = 500.0 * (testNow - at) / 3600000;
        near(f.energy._batteryChargeWh, charged);
        f.setPower(-250);
        f.pass();
        const auto dischargedAt = testNow;
        const double prior = f.energy._batteryChargeWh;
        f.pass();
        near(f.energy._batteryChargeWh, prior);
        // Existing SMG Diematic rounding turns -5.0 A into -4.99 A. This
        // timing change preserves that decoder behaviour; it is not meter calibration.
        const double decodedWatts = index == 0 ? 249.5 : 250;
        near(f.energy._batteryDischargeWh, decodedWatts * (testNow - dischargedAt) / 3600000);
    }
    std::puts("PASS: zero is a valid sample; net/separate currents preserve charge and discharge "
              "directions.");
}

static void restartStaticAndLive()
{
    Fixture f(profiles[0], 1000);
    f.driver.static_info = {&staticData, profile_smg::restart_static,
        sizeof(profile_smg::restart_static) / sizeof(modbus_register_t), 0};
    f.pass();
    const auto old = f.driver.batterySampleTiming();
    assert(old.available && old.sequence > 0 && f.driver.hasCompleteDynamicStateUpdate());
    std::vector<bool> classifications;
    f.driver.callback([&]() {
        // requestStaticData is already false for both completions, so it is
        // insufficient to classify which pass actually finished.
        assert(!f.driver.requestStaticData);
        classifications.push_back(f.driver.hasCompleteDynamicStateUpdate());
        f.publish();
    });
    for (bool interruptedStatic : {false, true})
    {
        const auto sequence = f.driver._batterySample.sequence;
        f.driver.requestStaticData = interruptedStatic;
        f.driver.static_info.curr_register = f.driver.static_info.array_size / 2;
        f.driver.live_info.curr_register = f.driver.live_info.array_size / 2;
        f.driver._pendingBatterySample = f.driver._batterySample;
        f.driver._pendingBatterySample.available = true;
        f.driver._batteryRequiredFields = f.driver._batteryReadFields = 0xff;
        f.driver._powmrLivePassCompleted = true;
        f.driver.restartStateRead();
        assert(f.driver.requestStaticData && f.driver.static_info.curr_register == 0 && f.driver.live_info.curr_register == 0);
        assert(!f.driver.hasCompleteDynamicStateUpdate() && !f.driver.consumePowMrLivePassCompleted());
        assert(!f.driver._pendingBatterySample.available && !f.driver._batteryReadFields && !f.driver._batteryRequiredFields);
        assert(!f.driver._batteryPassStartedAtMs && !f.driver._batteryOldestReadAtMs);
        assert(!f.driver._mCom._cacheValid && !f.driver._mCom.cachedReadAtMs());
        assert(!f.driver.batterySampleTiming().available && f.driver._batterySample.sequence == sequence);
        f.driver.acceptBatterySample();
        assert(f.driver._batterySample.sequence == sequence && !f.driver.batterySampleTiming().available);
        const size_t previousReads = wire.reads.size();
        const size_t previousCallbacks = classifications.size();
        for (size_t i = 0; i < f.driver.static_info.array_size; ++i) f.tick();
        assert(wire.reads.size() > previousReads);
        assert(wire.reads[previousReads].start == profile_smg::restart_static[0].read_block_start);
        assert(classifications.size() == previousCallbacks + 1 && !classifications.back());
        assert(!f.driver.batterySampleTiming().available && f.driver._batterySample.sequence == sequence);
        f.pass();
        assert(classifications.size() == previousCallbacks + 2 && classifications.back());
        assert(f.driver.batterySampleTiming().available && f.driver._batterySample.sequence == sequence + 1);
    }
    std::puts("PASS: interrupted static/live restart resets indices, cached block and pending tuple; static callback false then fresh dynamic true.");
}

static void deferredPowMrRestart()
{
    Fixture f(profiles[3], 1000);
    std::vector<bool> classifications;
    f.driver.callback([&]() { classifications.push_back(f.driver.hasCompleteDynamicStateUpdate()); });
    for (size_t i = 0; i < f.profile.count; ++i) { testNow += f.step; f.driver.loop(); }
    assert(classifications.empty());
    assert(f.driver.hasCompleteDynamicStateUpdate() && f.driver._powmrLivePassCompleted);
    assert(f.driver._pendingBatterySample.available);
    f.driver.restartStateRead();
    assert(!f.driver.hasCompleteDynamicStateUpdate() && !f.driver.consumePowMrLivePassCompleted());
    assert(!f.driver._pendingBatterySample.available);
    testNow += f.step; f.driver.loop(); // empty mock static table still emits a static completion
    assert(classifications.size() == 1 && !classifications.back());
    for (size_t i = 0; i < f.profile.count; ++i) { testNow += f.step; f.driver.loop(); }
    assert(classifications.size() == 1 && f.driver.hasCompleteDynamicStateUpdate());
    assert(f.driver.consumePowMrLivePassCompleted());
    assert(f.driver.hasCompleteDynamicStateUpdate()); // classification survives deferred consumption
    assert(!f.driver.consumePowMrLivePassCompleted());
    f.driver.acceptBatterySample();
    assert(f.driver.batterySampleTiming().available && f.driver._batterySample.sequence == 1);
    std::puts("PASS: restart discards unconsumed PowMr completion; new live classification precedes and survives deferred consume.");
}

int main()
{
    slowCycles();
    failureFreshness();
    timestampsAndBounds();
    zeroAndDirections();
    restartStaticAndLive();
    deferredPowMrRestart();
    std::puts("Modbus battery regressions passed.");
}
