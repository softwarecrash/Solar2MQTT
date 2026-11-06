#pragma once

// Transport remains the fixture's real scheduler. Only a JSON allocator fails;
// successful inverter replies are the existing MAX/GK/PI41/REVO fixtures.
class SnapshotTestAllocator : public ArduinoJson::Allocator
{
public:
    bool failing = false;
    void *allocate(size_t size) override { return failing ? nullptr : std::malloc(size); }
    void deallocate(void *pointer) override { std::free(pointer); }
    void *reallocate(void *pointer, size_t size) override
    {
        return failing ? nullptr : std::realloc(pointer, size);
    }
};

static void snapshotFailureFreshness()
{
    for (unsigned variant = 0; variant < 5; ++variant)
    {
        const protocol_type_t protocol = variant == 0 ? PI30_MAX : variant == 1 ? PI30_PIP_GK
                                                        : variant == 2 ? PI41 : PI30_REVO;
        SnapshotTestAllocator snapshotAllocator, damagedAllocator;
        Fixture f(protocol);
        if (variant == 4)
            replies["QALL"] = "NAK";
        f.cycles(2);
        JsonDocument allocated(&snapshotAllocator);
        assert(allocated.set(f.state._inverterSnapshot));
        swap(f.state._inverterSnapshot, allocated);
        allocated.clear();

        const auto before = f.service.batterySampleTiming(testNow);
        assert(before.available);
        EnergyBacklog energy;
        energy.updateBatteryEnergy(f.state, before, testNow);
        // A driver may finish a fresh reading while its publication copy fails.
        snapshotAllocator.failing = true;
        f.cycles(1);
        assert(!g_stateDoc.overflowed());
        assert(f.driver.batterySampleTiming().sequence != before.sequence);
        const auto held = f.service.batterySampleTiming(testNow);
        assert(held.sequence == before.sequence && held.acceptedAtMs == before.acceptedAtMs &&
               held.sampledAtMs == before.sampledAtMs && held.validForMs == before.validForMs);
        energy.updateBatteryEnergy(f.state, held, testNow);
        assert(energy._batterySample.sequence == before.sequence);
        snapshotAllocator.failing = false;
        f.cycles(1);
        const auto committed = f.service.batterySampleTiming(testNow);
        assert(committed.sequence == f.driver.batterySampleTiming().sequence && committed.sequence != before.sequence);
        energy.updateBatteryEnergy(f.state, committed, testNow);
        assert(energy._batterySample.sequence == committed.sequence);

        // A damaged producer must be rebuilt, not copied into the good cache.
        damagedAllocator.failing = true;
        JsonDocument damaged(&damagedAllocator);
        damaged["allocation-fault"] = true;
        assert(damaged.overflowed());
        swap(g_stateDoc, damaged);
        f.state.refreshBindings();
        f.driver.notifyStateUpdate(true);
        assert(!g_stateDoc.overflowed() && f.service._awaitingFreshState && f.driver.stateReadRetryPending);
        assert(f.service.batterySampleTiming(testNow).sequence == committed.sequence);

        const auto healthyReplies = replies;
        for (const char *command : {"QPI", "QPIRI", "QMN", "QFLAG", "QMD"})
            replies[command] = "NAK";
        f.cycles(1);
        assert(f.service._awaitingFreshState && !f.service._recoveryStaticComplete);
        assert(f.service.batterySampleTiming(testNow).sequence == committed.sequence);
        replies = healthyReplies;
        for (unsigned guard = 0; !f.service._recoveryStaticComplete && guard < 80; ++guard)
            f.poll();
        assert(f.service._recoveryStaticComplete);
        // A successful status-only cycle must not bless the restored battery.
        replies["QPIGS"] = "NOA";
        replies["QALL"] = "NAK";
        f.cycles(1);
        assert(f.service._awaitingFreshState);
        assert(f.service.batterySampleTiming(testNow).sequence == committed.sequence);
        replies = healthyReplies;
        // A fresh post-recovery tuple whose copy fails must not become the
        // successful recovery just because a later status-only cycle finishes.
        snapshotAllocator.failing = true;
        f.cycles(1);
        assert(f.service._awaitingFreshState);
        assert(f.service.batterySampleTiming(testNow).sequence == committed.sequence);
        snapshotAllocator.failing = false;
        replies["QPIGS"] = "NOA";
        replies["QALL"] = "NAK";
        f.cycles(1);
        assert(f.service._awaitingFreshState);
        assert(f.service.batterySampleTiming(testNow).sequence == committed.sequence);
        replies = healthyReplies;
        f.cycles(1);
        assert(!f.service._awaitingFreshState && f.service.hasFreshBatterySample(testNow));
        assert(f.service.batterySampleTiming(testNow).sequence == f.driver.batterySampleTiming().sequence);
        const auto recovered = f.snapshot();
        assert(!recovered["LiveData"][DESCR_PV2_Input_Voltage].isNull());
    }
    std::puts("PASS: MAX/GK/PI41/REVO with/without QALL: failed copies keep battery timestamps, "
              "source overflow restarts full reads, and static/status-only replies cannot complete recovery.");
}
