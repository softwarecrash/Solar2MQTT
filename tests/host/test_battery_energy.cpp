#include "energy_fixture.h"
#include "battery_energy_cases.h"

static void failedCheckpointRetryBound()
{
    using namespace battery_cases;
    reset();
    SolarState state;
    state.begin();
    EnergyBacklog energy;
    energy.begin();
    sampleAt(energy, state, 100, 1, 500);
    storage.failFsWrites = storage.failNvsWrites = true;
    const unsigned before = storage.fsWrites;
    for (uint32_t now = 120100; now < 130100; now += 100)
    {
        testNow = now;
        energy.updateBatteryEnergy(state, reading(100, 1), now);
    }
    assert(storage.fsWrites == before + 1);
    testNow = 130100;
    energy.updateBatteryEnergy(state, reading(100, 1), testNow);
    assert(storage.fsWrites == before + 2);
    storage.failFsWrites = storage.failNvsWrites = false;
    testNow = 140100;
    energy.updateBatteryEnergy(state, reading(100, 1), testNow);
    assert(!energy._batteryDirty);
    std::puts("PASS: failed checkpoints retry at most once per ten seconds and recover.");
}

int main()
{
    battery_cases::timedIntegration();
    failedCheckpointRetryBound();
}
