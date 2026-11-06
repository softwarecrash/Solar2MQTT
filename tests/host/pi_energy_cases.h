#pragma once

static void freshnessAndEnergy()
{
    Fixture f(PI30_PIP_GK);
    assert(!f.service.hasFreshBatterySample(testNow));
    f.cycles(2);
    assert(f.driver.connectionHoldMs() > 10000);
    assert(f.service.hasFreshBatterySample(testNow));
    // Use a known completed battery sample: 50 V * 10 A = 500 W.
    liveData[DESCR_Battery_Voltage] = 50;
    liveData[DESCR_Battery_Charge_Current] = 10;
    liveData[DESCR_Battery_Discharge_Current] = 0;
    f.driver.lastBatterySampleAt = testNow;
    f.driver.lastBatteryAcceptedAt = testNow;
    f.driver.lastBatteryValidForMs = 24000;
    ++f.driver.batterySampleSequence;
    f.driver.lastCycleDurationMs = 11000;
    f.driver.notifyStateUpdate(true);
    EnergyBacklog energy;
    energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    const uint32_t begin = testNow;
    for (unsigned i = 1; i <= 60; i++)
    {
        testNow = begin + i * 1000;
        if (i % 11 == 0)
        {
            f.driver.lastBatterySampleAt = testNow;
            f.driver.lastBatteryAcceptedAt = testNow;
            ++f.driver.batterySampleSequence;
            f.driver.haveBatterySample = true;
            f.driver.notifyStateUpdate(true);
        }
        // A display flag must not gate a still-valid committed battery sample.
        f.driver.connection = (i % 11) <= 5;
        energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    }
    near(energy._batteryChargeWh, 500.0 / 60.0);
    // The same exact interval must work in the opposite direction.
    liveData[DESCR_Battery_Charge_Current] = 0;
    liveData[DESCR_Battery_Discharge_Current] = 10;
    f.driver.lastBatterySampleAt = testNow;
    f.driver.lastBatteryAcceptedAt = testNow;
    ++f.driver.batterySampleSequence;
    f.driver.notifyStateUpdate(true);
    EnergyBacklog discharge;
    discharge.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    for (unsigned i = 0; i < 60; i++)
    {
        testNow += 1000;
        f.driver.lastBatterySampleAt = testNow;
        f.driver.lastBatteryAcceptedAt = testNow;
        ++f.driver.batterySampleSequence;
        f.driver.notifyStateUpdate(true);
        discharge.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    }
    near(discharge._batteryDischargeWh, 500.0 / 60.0);
    near(discharge._batteryChargeWh, 0);
    EnergyBacklog expiry;
    expiry.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    const uint32_t lease = f.driver.lastBatteryValidForMs;
    testNow += lease + 1;
    assert(!f.service.hasFreshBatterySample(testNow));
    expiry.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    near(expiry._batteryDischargeWh, 500.0 * lease / 3600000.0);
    const double before = expiry._batteryDischargeWh;
    testNow += 10000;
    expiry.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    near(expiry._batteryDischargeWh, before);
    // Fresh warnings/secondary replies must not revive an old battery reading.
    for (auto &r : replies)
        r.second = "NAK";
    replies["QPIRI"] = gkRating;
    replies["QPIWS"] = std::string(38, '0');
    for (unsigned i = 0; i < 24; i++)
        f.poll();
    assert(f.driver.connection);
    assert(!f.service.hasFreshBatterySample(testNow));
    assert(f.driver.lastBatteryValidForMs == lease);
    replies["QPIWS"] = "NAK";
    for (unsigned i = 0; i < 100; i++)
        f.poll();
    assert(!f.driver.connection);
    assert(!f.service.hasFreshBatterySample(testNow));
    // Clock wraparound and bounded slow-cycle allowance.
    f.driver.lastCycleDurationMs = 1000000;
    assert(f.driver.connectionHoldMs() == 120000);
    f.driver.delayTime = 1000;
    f.driver.lastCycleDurationMs = 11000;
    assert(f.driver.connectionHoldMs() == 27000); // covers 0.1-second next-cycle jitter
    f.driver.delayTime = 0xffffffffUL;
    assert(f.driver.connectionHoldMs() == 120000);
    f.driver.delayTime = 1000;
    f.driver.lastCycleDurationMs = 0;
    f.driver.lastBatterySampleAt = 0xfffffff0UL;
    f.driver.haveBatterySample = true;
    assert(f.driver.hasFreshBatterySample(0x00000010UL));
    f.driver.suspendSerial = true;
    assert(!f.driver.hasFreshBatterySample(0x00000010UL));
    std::puts("PASS: 11-second polling, exact 500 W integration, stale-battery expiry despite live "
              "warnings, total loss, timeout bound, suspend and 32-bit clock wrap.");
}

static void pollingEnergyAndFormats()
{
    Fixture f(PI30_PIP_GK);
    replies["QPIGS"] = fields(fields(fields(gkDay, 21, 8, "50.00"), 21, 9, "010"), 21, 15, "00000");
    f.cycles(2);
    EnergyBacklog energy;
    energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    const uint32_t start = testNow;
    for (unsigned i = 0; i < 80; i++)
    {
        f.poll();
        assert(f.driver.connection);
        assert(f.service.hasFreshBatterySample(testNow));
        energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow), testNow);
    }
    near(energy._batteryChargeWh,
         500.0 * static_cast<double>(static_cast<uint32_t>(testNow - start)) / 3600000.0);
    for (auto p : {PI41, PI30_MAX})
    {
        f.driver.protocol = p;
        f.driver.forcedProtocol = p;
        replies["QPIRI"] = p == PI41 ? gkRating : maxRating;
        replies["QPIGS"] = p == PI41 ? fields(gkDay, 21, 12, "04.0") : maxDay;
        replies["QPIGS2"] = p == PI41 ? pi41Pv2 : "01.9 388.9 00745";
        f.cycles(2);
        auto d = f.snapshot();
        assert(d["LiveData"][DESCR_Battery_SCC2_Volt].isNull());
        assert(d["LiveData"][DESCR_PV_Total_Input_Power].isNull());
        if (p == PI30_MAX)
            assert(d["LiveData"][DESCR_AC_Out_Voltage_L2].isNull());
    }
    // Genuine zero production remains a value, not an absent/failed sample.
    replies["QPIGS2"] = "00.0 000.0 00000";
    f.cycles(2);
    auto night = f.snapshot();
    assert(!night["LiveData"][DESCR_PV2_Input_Power].isNull());
    near(night["LiveData"][DESCR_PV2_Input_Power].as<double>(), 0);
    std::puts("PASS: continuous integration through real polling, 12/11/3-field transitions, L2 "
              "cleanup and genuine night zeros.");
}
