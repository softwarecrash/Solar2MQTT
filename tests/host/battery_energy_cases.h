#pragma once
#include "battery_test_helpers.h"
namespace battery_cases
{
static void timedIntegration()
{
    // Each minute contains 60 * 400 ms at 50 W, regardless of one-second phase.
    for (unsigned phase : {0U, 13U, 499U, 999U})
    {
        reset();
        SolarState state;
        state.begin();
        EnergyBacklog e;
        e.begin();
        uint32_t seq = 0;
        bool previous = false;
        BatterySampleTiming current;
        for (uint32_t t = 0; t <= 60000; t++)
        {
            const bool on = (t + phase) % 1000 < 400;
            const bool changed = t == 0 || on != previous;
            if (changed)
            {
                current = reading(100 + t, ++seq);
                power(state, on ? 50 : 0);
                previous = on;
            }
            if (changed || t % 31 == 0 || t == 60000)
            {
                testNow = 100 + t;
                e.updateBatteryEnergy(state, current, testNow);
            }
        }
        near(e._batteryChargeWh, 1.0 / 3);
        near(e._batteryDischargeWh, 0);
        assert(storage.fsWrites <= 1);
    }
    reset();
    SolarState state;
    state.begin();
    EnergyBacklog irregular;
    irregular.begin();
    sampleAt(irregular, state, 100, 1, 500);
    sampleAt(irregular, state, 471, 2, -250);
    sampleAt(irregular, state, 1266, 3, 0);
    sampleAt(irregular, state, 2001, 4, 100);
    testNow = 2251;
    irregular.updateBatteryEnergy(state, reading(2001, 4), testNow);
    near(irregular._batteryChargeWh, (500 * 371.0 + 100 * 250.0) / 3600000);
    near(irregular._batteryDischargeWh, 250 * 795.0 / 3600000);
    reset();
    state.begin();
    EnergyBacklog delayed;
    power(state, 500);
    testNow = 4000;
    auto s = reading(1000, 1, 6000);
    s.acceptedAtMs = 3000;
    delayed.updateBatteryEnergy(state, s, testNow);
    testNow = 11000;
    delayed.updateBatteryEnergy(state, s, testNow);
    near(delayed._batteryChargeWh, 500 * 3000.0 / 3600000);
    assert(state.doc()["LiveData"][DESCR_Battery_Charging_Power_Calculated] == 0);
    const double stopped = delayed._batteryChargeWh;
    testNow = 20000;
    delayed.updateBatteryEnergy(state, s, testNow);
    near(delayed._batteryChargeWh, stopped);
    reset();
    state.begin();
    EnergyBacklog wrap;
    sampleAt(wrap, state, 0xfffffff0UL, 1, 500);
    testNow = 0x1f0;
    wrap.updateBatteryEnergy(state, reading(0xfffffff0UL, 1), testNow);
    near(wrap._batteryChargeWh, 500 * 512.0 / 3600000);
    const double before = wrap._batteryChargeWh;
    testNow = 100;
    wrap.updateBatteryEnergy(state, reading(100, 2), testNow);
    near(wrap._batteryChargeWh, before);
    reset();
    state.begin();
    EnergyBacklog malformed;
    for (unsigned i = 0; i < 4; i++)
    {
        power(state, 500);
        auto live = state.doc()["LiveData"].as<JsonObject>();
        if (i == 0)
            live[DESCR_Battery_Voltage] = "50";
        if (i == 1)
            live[DESCR_Battery_Charge_Current] = -1;
        if (i == 2)
            live[DESCR_Battery_Discharge_Current] = std::nan("");
        if (i == 3)
            live[DESCR_Battery_Voltage] = true;
        state.commitInverterState();
        testNow = 1000 + i * 1000;
        malformed.updateBatteryEnergy(state, reading(testNow, i + 1), testNow);
        testNow += 500;
        malformed.updateBatteryEnergy(state, reading(testNow - 500, i + 1), testNow);
    }
    near(malformed._batteryChargeWh, 0);
    near(malformed._batteryDischargeWh, 0);
    auto future = reading(testNow + 1000, 5);
    power(state, 500);
    malformed.updateBatteryEnergy(state, future, testNow);
    testNow += 500;
    malformed.updateBatteryEnergy(state, future, testNow);
    near(malformed._batteryChargeWh, 0);
    reset();
    state.begin();
    EnergyBacklog paused;
    sampleAt(paused, state, 100, 1, 500);
    testNow = 1100;
    auto unavailable = reading(100, 1);
    unavailable.available = false;
    paused.updateBatteryEnergy(state, unavailable, testNow);
    const auto pausedWh = paused._batteryChargeWh;
    testNow = 2100;
    paused.updateBatteryEnergy(state, reading(100, 1), testNow);
    near(paused._batteryChargeWh, pausedWh);
    sampleAt(paused, state, 2100, 2, 500);
    testNow = 3100;
    paused.updateBatteryEnergy(state, reading(2100, 2), testNow);
    near(paused._batteryChargeWh, pausedWh + 500.0 / 3600);
    std::puts("PASS: phase-independent 400 ms pulses, irregular/directional readings, UART delay, "
              "expiry, wrap, invalid values and bounded flash writes.");
}
} // namespace battery_cases
