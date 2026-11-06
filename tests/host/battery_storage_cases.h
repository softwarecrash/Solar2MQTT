#pragma once
#include "battery_test_helpers.h"
namespace battery_cases
{
inline JsonDocument diagnostics(EnergyBacklog &e)
{
    JsonDocument d;
    e.writeBatteryDiagnostics(d.to<JsonObject>(), testNow);
    return d;
}
static void persistence()
{
    reset();
    EnergyBacklog normal;
    normal.begin();
    assert(normal._ready && !normal.batteryMigrationPending());
    counters(normal, 10.25, 20.5);
    assert(normal.flushBatteryEnergy());
    EnergyBacklog reboot;
    reboot.begin();
    near(reboot._batteryChargeWh, 10.25);
    near(reboot._batteryDischargeWh, 20.5);
    auto diag = diagnostics(reboot);
    assert(diag["partitions"]["runningApp"]["address"] == 0x10000);
    assert(diag["partitions"]["backlog"]["present"] == true);
    assert(diag["storage"]["backend"] == "littlefs");
    counters(reboot, 10.250001, 20.500001);
    assert(reboot.flushBatteryEnergy());
    EnergyBacklog quantized;
    quantized.begin();
    assert(quantized._batteryChargeWh >= 10.250001 && quantized._batteryChargeWh < 10.251001);
    assert(quantized._batteryDischargeWh >= 20.500001);
    reset();
    const auto old = record(8, 12345, 67890, true);
    file("/battery_a.bin", old);
    file("/battery_b.bin", record(9, 45678, 90123, true));
    EnergyBacklog imported;
    imported.begin();
    near(imported._batteryChargeWh, 45.678);
    near(imported._batteryDischargeWh, 90.123);
    assert(*storage.files.at("/battery_a.bin") == old);
    assert(!imported.batteryMigrationPending());
    EnergyBacklog importedAgain;
    importedAgain.begin();
    near(importedAgain._batteryDischargeWh, 90.123);
    reset(false);
    EnergyBacklog fallback;
    fallback.begin();
    assert(!fallback._ready && fallback.batteryMigrationPending());
    String error;
    assert(fallback.applyBatteryMigration(id, 1.25, 2.5, testNow, error));
    EnergyBacklog legacyReboot;
    legacyReboot.begin();
    near(legacyReboot._batteryChargeWh, 1250);
    near(legacyReboot._batteryDischargeWh, 2500);
    assert(!legacyReboot.batteryMigrationPending());
    assert(diagnostics(legacyReboot)["storage"]["backend"] == "nvs");
    assert(legacyReboot.applyBatteryMigration(id, 1.25, 2.5, testNow, error));
    near(legacyReboot._batteryChargeWh, 1250);
    assert(!legacyReboot.applyBatteryMigration(otherId, 1, 1, testNow, error));
    storage.partitionPresent = true;
    file("/battery_v2_a.bin", record(999, 999000, 999000));
    EnergyBacklog fsReturns;
    fsReturns.begin();
    near(fsReturns._batteryChargeWh, 1250);
    assert(diagnostics(fsReturns)["storage"]["backend"] == "nvs");
    // A fallback from an already mounted FS has a shared sequence and can be
    // copied back safely. A recovery from an inaccessible FS remains in NVS.
    reset();
    EnergyBacklog modern;
    modern.begin();
    counters(modern, 1250, 2500);
    assert(modern.flushBatteryEnergy());
    storage.failFsWrites = true;
    counters(modern, 1251, 2502);
    assert(modern.flushBatteryEnergy());
    assert(diagnostics(modern)["storage"]["backend"] == "nvs");
    storage.failFsWrites = false;
    EnergyBacklog afterFallback;
    afterFallback.begin();
    near(afterFallback._batteryChargeWh, 1251);
    for (bool erased : {false, true})
    {
        reset();
        storage.mountFails = true;
        storage.erased = erased;
        file("/keep", {1, 2, 3});
        EnergyBacklog mount;
        mount.begin();
        assert(storage.implicitFormatCalls == 0);
        assert(storage.formatCalls == (erased ? 1U : 0U));
        if (!erased)
            assert(storage.files.count("/keep"));
    }
    reset();
    storage.mountFails = storage.erased = storage.flashReadFails = true;
    EnergyBacklog unreadable;
    unreadable.begin();
    assert(storage.formatCalls == 0);
    reset();
    storage.nvsAvailable = false;
    EnergyBacklog fsOnly;
    fsOnly.begin();
    counters(fsOnly, 4, 5);
    assert(fsOnly.flushBatteryEnergy());
    reset(false);
    storage.nvsAvailable = false;
    EnergyBacklog nowhere;
    nowhere.begin();
    assert(!nowhere.flushBatteryEnergy());
    assert(nowhere.batteryMigrationPending());
    assert(diagnostics(nowhere)["storage"]["error"] == "battery_storage_unavailable");
    reset(false);
    storage.failBoolWrites = true;
    EnergyBacklog markerFails;
    markerFails.begin();
    assert(markerFails.batteryMigrationPending());
    assert(!markerFails.flushBatteryEnergy());
    assert(diagnostics(markerFails)["storage"]["warning"] == "nvs_backend_marker_write_failed");
    std::puts("PASS: legacy/V2 import, missing markers, LittleFS/NVS fallback, safe formatting and "
              "unavailable storage.");
}

static void interruptedWritesAndReset()
{
    for (bool fs : {false, true})
    {
        reset(fs);
        EnergyBacklog good;
        good.begin();
        String error;
        if (good.batteryMigrationPending())
            assert(good.applyBatteryMigration(id, 0, 0, testNow, error));
        counters(good, 12, 34);
        assert(good.flushBatteryEnergy());
        const auto seq = good._batteryCheckpointSequence;
        storage.fileWriteLimit = storage.nvsWriteLimit = 7;
        for (int i = 0; i < 4; i++)
        {
            counters(good, 999 + i, 999);
            assert(!good.flushBatteryEnergy());
        }
        assert(good._batteryCheckpointSequence == seq);
        storage.fileWriteLimit = storage.nvsWriteLimit = std::numeric_limits<size_t>::max();
        EnergyBacklog reboot;
        reboot.begin();
        near(reboot._batteryChargeWh, 12);
        near(reboot._batteryDischargeWh, 34);
    }
    reset();
    file("/battery_v2_a.bin", record(0xffffffffUL, 1000, 2000));
    file("/battery_v2_b.bin", record(0, 3000, 4000));
    EnergyBacklog wrapped;
    wrapped.begin();
    near(wrapped._batteryChargeWh, 3);
    (*storage.files["/battery_v2_b.bin"])[12] ^= 1;
    EnergyBacklog corrupted;
    corrupted.begin();
    near(corrupted._batteryChargeWh, 1);
    reset(false);
    storage.blobs["batenergy:counter_a"] = {1, 2, 3};
    EnergyBacklog corrupt;
    corrupt.begin();
    assert(corrupt._batteryRecoveryRequired);
    const auto before = storage.blobs;
    counters(corrupt, 1, 2);
    assert(!corrupt.flushBatteryEnergy());
    assert(storage.blobs == before);
    reset();
    file("/battery_v2_a.bin", record(100, 900000, 800000));
    storage.blobs["batenergy:counter_a"] = record(200, 700000, 600000);
    storage.flags["batenergy:factory_reset"] = true;
    EnergyBacklog factory;
    factory.begin();
    near(factory._batteryChargeWh, 0);
    assert(!factory._batteryFactoryResetPending && storage.flags["batenergy:prefer_nvs"]);
    file("/battery_v2_b.bin", record(500, 999000, 999000));
    EnergyBacklog resetReboot;
    resetReboot.begin();
    near(resetReboot._batteryChargeWh, 0);
    reset(false);
    storage.blobs["batenergy:counter_a"] = record(300, 700000, 600000);
    storage.flags["batenergy:factory_reset"] = true;
    storage.failRemoves = true;
    EnergyBacklog failedDelete;
    failedDelete.begin();
    assert(failedDelete._batteryFactoryResetPending && failedDelete.batteryMigrationPending());
    assert(!failedDelete.flushBatteryEnergy());
    storage.failRemoves = false;
    EnergyBacklog retry;
    retry.begin();
    near(retry._batteryChargeWh, 0);
    assert(!retry._batteryFactoryResetPending);
    std::puts("PASS: repeated torn writes retain the last good slot, CRC/sequence wrap, corrupt "
              "recovery and deliberate factory reset.");
}

static void migration()
{
    reset(false);
    SolarState state;
    state.begin();
    EnergyBacklog e;
    e.begin();
    sampleAt(e, state, 100, 1, 500);
    testNow = 3700;
    e.updateBatteryEnergy(state, reading(100, 1), testNow);
    near(e._batteryChargeWh, .5);
    JsonDocument visible;
    state.snapshotTo(visible);
    assert(visible["LiveData"][DESCR_Battery_Charge_Energy].isNull());
    assert(e.flushBatteryEnergy());
    EnergyBacklog session;
    session.begin();
    assert(session.batteryMigrationPending());
    near(session._batteryChargeWh, .5);
    String error;
    assert(session.applyBatteryMigration(id, .369740764, 2.515558972, testNow, error));
    near(session._batteryChargeWh, 370.240764);
    assert(session.applyBatteryMigration(id, .369740764, 2.515558972, testNow, error));
    near(session._batteryChargeWh, 370.240764);
    EnergyBacklog finalBoot;
    finalBoot.begin();
    near(finalBoot._batteryChargeWh, 370.241);
    assert(finalBoot.batteryMigrationReceipt() == id);
    assert(finalBoot.applyBatteryMigration(id, .369740764, 2.515558972, testNow, error));
    near(finalBoot._batteryChargeWh, 370.241);
    for (bool ambiguous : {false, true})
    {
        reset(false);
        EnergyBacklog failed;
        failed.begin();
        counters(failed, 10, 20);
        if (ambiguous)
            storage.failNvsReads = 1;
        else
            storage.failNvsWrites = true;
        assert(!failed.applyBatteryMigration(id, 1, 2, testNow, error));
        assert(failed._batteryMigrationUncertain && failed.batteryMigrationPending());
        near(failed._batteryChargeWh, 10);
        const auto writes = storage.nvsWrites;
        assert(!failed.flushBatteryEnergy());
        assert(storage.nvsWrites == writes);
        assert(!failed.applyBatteryMigration(otherId, 1, 2, testNow, error));
        assert(!failed.applyBatteryMigration(id, 2, 2, testNow, error));
        storage.failNvsWrites = false;
        if (ambiguous)
        {
            EnergyBacklog boot;
            boot.begin();
            near(boot._batteryChargeWh, 1010);
            assert(boot.applyBatteryMigration(id, 1, 2, testNow, error));
            near(boot._batteryChargeWh, 1010);
        }
        else
        {
            assert(failed.applyBatteryMigration(id, 1, 2, testNow, error));
            near(failed._batteryChargeWh, 1010);
        }
    }
    reset(false);
    EnergyBacklog invalid;
    invalid.begin();
    for (const char *bad :
         {"", "00000000000000000000000000000000", "123", "ABCDEF1234567890ABCDEF1234567890AB"})
        assert(!invalid.applyBatteryMigration(bad, 0, 0, testNow, error));
    for (double bad : {-1.0, 1.0e10, std::nan(""), std::numeric_limits<double>::infinity()})
        assert(!invalid.applyBatteryMigration(id, bad, 0, testNow, error));
    counters(invalid, 1, 0);
    assert(!invalid.applyBatteryMigration(id, 1.0e9, 0, testNow, error));
    std::puts("PASS: pending session survives reboot, baseline adds once, ambiguous writes, "
              "idempotent retries and migration bounds.");
}
static void boundedCheckpoints()
{
    SolarState state;
    reset();
    state.begin();
    EnergyBacklog bounded;
    bounded.begin();
    for (unsigned i = 0; i <= 900; i++)
        sampleAt(bounded, state, 100 + i * 400, i + 1, i % 2 ? 0 : 50);
    assert(storage.fsWrites == 4);
    assert(bounded.flushBatteryEnergy());
    assert(storage.fsWrites <= 5);
    storage.failFsWrites = storage.failNvsWrites = true;
    const auto writes = storage.fsWrites;
    for (unsigned i = 0; i <= 300; i++)
        sampleAt(bounded, state, 480100 + i * 100, i + 1000, 50);
    assert(storage.fsWrites - writes == 4); // failed checkpoints retry every ten seconds
    std::puts("PASS: checkpoint/retry bounds under repeated direction changes.");
}
} // namespace battery_cases
