#pragma once

static void firstCycleAndRevoFallback()
{
    for (auto target : {PI30_MAX, PI30_PIP_GK})
        for (auto initial : {PI30, PI30_UNKNOWN})
        {
            Fixture f(target);
            f.driver.protocol = initial;
            f.driver.forcedProtocol = NoD;
            f.cycles(2);
            assert(f.driver.protocol == target);
            assert(f.service.hasFreshBatterySample(testNow));
        }
    Fixture f(PI30_REVO);
    replies["QALL"] = "NAK";
    replies["QPIWS"] = std::string(38, '0');
    f.cycles(3);
    auto d = f.snapshot();
    assert(std::strcmp(d["LiveData"][DESCR_Fault_Code].as<const char *>(), "Ok") == 0);
    assert(!d["LiveData"][DESCR_PV2_Input_Power].isNull());
    std::puts(
        "PASS: cold PI30/UNKNOWN detection for MAX/GK and complete REVO cycles without QALL.");
}

static void lateStartupRefinement()
{
    unsigned cases = 0;
    for (auto initial : {PI30, PI30_UNKNOWN})
        for (const char *rejected : {"NAK", "NOA"})
            for (const char *current : {"0002", "02.0", "0000"})
                for (bool json : {false, true})
                {
                    Fixture f(PI30_PIP_GK);
                    f.driver.protocol = initial;
                    f.driver.forcedProtocol = NoD;
                    _settings.get.json = json;
                    replies["QPIRI"] = rejected;
                    for (unsigned guard = 0; f.driver.requestStaticData && guard < 20; guard++)
                        f.poll();
                    assert(!f.driver.requestStaticData && f.dynamicCompleted == 0);
                    const auto staticCompleted = f.completed;
                    const bool night = std::strcmp(current, "0000") == 0;
                    std::string frame = fields(fields(gkDay, 21, 9, "000"), 21, 15, "00007");
                    frame = fields(frame, 21, 12, current);
                    if (night)
                        frame = fields(fields(frame, 21, 13, "000.0"), 21, 19, "00000");
                    replies["QPIGS"] = frame;
                    replies["QPIRI"] = gkRating;
                    auto checkPublished = [&]
                    {
                        const auto snapshot = f.snapshot();
                        const char *raw = snapshot["RawData"]["QPIGS"].as<const char *>();
                        if (!raw || !*raw)
                            return;
                        for (const char *key :
                             {DESCR_Battery_Discharge_Current, DESCR_Status_Flag,
                              DESCR_Battery_Voltage_Offset_Fans_On, DESCR_EEPROM_Version,
                              DESCR_PV_Charging_Power, DESCR_PV1_Input_Current})
                            assert(!snapshot["LiveData"][key].isNull());
                        near(snapshot["LiveData"][DESCR_Battery_Discharge_Current].as<double>(), 7);
                        near(snapshot["LiveData"][DESCR_Battery_Load].as<double>(), -7);
                        near(snapshot["LiveData"][DESCR_PV_Charging_Power].as<double>(),
                             night ? 0 : 93);
                        assert(snapshot["LiveData"][DESCR_PV_Generation_Day].isNull());
                        assert(f.publication(DESCR_Battery_Discharge_Current) == "7");
                        assert(f.publication(DESCR_PV_Input_Power) != "<absent>");
                        assert(f.publication("PV1_Input_Power") == "<absent>");
                        if (json)
                        {
                            bool found = false;
                            for (const auto &message : f.mqtt._mqtt.messages)
                                if (message.topic == "solar/test/Data")
                                {
                                    JsonDocument decoded;
                                    assert(!deserializeJson(decoded, message.value));
                                    near(decoded["LiveData"][DESCR_Battery_Discharge_Current]
                                             .as<double>(),
                                         7);
                                    found = true;
                                }
                            assert(found);
                        }
                    };
                    whileWaiting = [&](const std::string &)
                    {
                        f.checkSnapshot();
                        checkPublished();
                    };
                    unsigned finishedCycles = 0;
                    for (unsigned guard = 0; f.dynamicCompleted == 0 && guard < 30; guard++)
                    {
                        const bool finishing = f.driver.requestCounter == 7;
                        f.poll();
                        checkPublished();
                        if (finishing)
                        {
                            ++finishedCycles;
                            if (f.dynamicCompleted == 0)
                            {
                                // A discarded layout must not become a fresh battery sample.
                                assert(f.driver.protocol == PI30_PIP_GK);
                                assert(!f.driver.haveBatterySample &&
                                       f.driver.batterySampleSequence == 0);
                                assert(!f.service.hasFreshBatterySample(testNow));
                                assert(std::string(f.snapshot()["RawData"]["QPIGS"] | "").empty());
                            }
                        }
                    }
                    whileWaiting = nullptr;
                    assert(f.dynamicCompleted == 1 && f.completed == staticCompleted + 1);
                    assert(finishedCycles == (std::strchr(current, '.') ? 1U : 2U));
                    assert(f.driver.batterySampleSequence == 1 &&
                           f.service.hasFreshBatterySample(testNow));
                    f.cycles(2);
                    checkPublished();
                    ++cases;
                }
    for (auto target : {PI30_MAX, PI30_PIP_GK, PI41})
        for (const char *rejected : {"NAK", "NOA"})
        {
            Fixture f(target);
            f.driver.protocol = PI30_UNKNOWN;
            f.driver.forcedProtocol = NoD;
            replies["QPIRI"] = rejected;
            for (unsigned guard = 0; f.driver.requestStaticData && guard < 20; guard++)
                f.poll();
            replies["QPIRI"] = target == PI30_MAX ? maxRating : gkRating;
            f.cycles(1);
            assert(f.driver.protocol == target && f.driver.batterySampleSequence == 1);
            const auto snapshot = f.snapshot();
            assert(!snapshot["LiveData"][DESCR_Battery_Discharge_Current].isNull());
            assert(!snapshot["LiveData"][DESCR_PV2_Input_Current].isNull());
            assert(!snapshot["LiveData"][DESCR_PV2_Input_Power].isNull());
            if (target == PI41)
                assert(!snapshot["LiveData"][DESCR_AC_Out_Voltage_L2].isNull());
            assert(snapshot["LiveData"]["PV1_Input_Power"].isNull());
            f.cycles(2);
            ++cases;
        }
    // The reverse transition also happens: late short QPIRI confirms REVO
    // after an earlier standard 21-field mapping. Keep QALL's authoritative data.
    for (auto initial : {PI30, PI30_UNKNOWN})
    {
        Fixture f(PI30_REVO);
        f.driver.protocol = initial;
        f.driver.forcedProtocol = NoD;
        replies["QPIRI"] = "NAK";
        for (unsigned guard = 0; f.driver.requestStaticData && guard < 20; guard++)
            f.poll();
        replies["QPIRI"] = fields(gkRating, 21);
        auto check = [&]
        {
            const auto snapshot = f.snapshot();
            const char *raw = snapshot["RawData"]["QPIGS"].as<const char *>();
            if (raw && *raw)
            {
                if (snapshot["LiveData"][DESCR_PV1_Input_Current].as<double>() != 15.3)
                    std::fprintf(stderr, "Late REVO initial=%d protocol=%d step=%u accepted=%u\n",
                                 initial, f.driver.protocol, f.driver.requestCounter,
                                 f.dynamicCompleted);
                near(snapshot["LiveData"][DESCR_PV_Input_Power].as<double>(), 622);
                near(snapshot["LiveData"][DESCR_PV_Charging_Power].as<double>(), 1385);
                near(snapshot["LiveData"][DESCR_PV1_Input_Current].as<double>(), 15.3);
            }
        };
        for (unsigned guard = 0; f.dynamicCompleted == 0 && guard < 30; guard++)
        {
            f.poll();
            check();
        }
        assert(f.driver.protocol == PI30_REVO && f.dynamicCompleted == 1);
        assert(f.driver.batterySampleSequence == 1);
        f.cycles(2);
        check();
        ++cases;
    }
    // A late transport failure must clear the provisional layout, preserve
    // the old snapshot, and allow a subsequent valid cycle to recover.
    for (const char *failure : {"ERCRC", "NOA", "NAK"})
    {
        Fixture f(PI30_PIP_GK);
        f.driver.protocol = PI30_UNKNOWN;
        f.driver.forcedProtocol = NoD;
        replies["QPIRI"] = "NAK";
        for (unsigned guard = 0; f.driver.requestStaticData && guard < 20; guard++)
            f.poll();
        replies["QPIWS"] = failure;
        for (unsigned i = 0; i < 3; i++)
        {
            for (unsigned guard = 0; f.driver.requestCounter != 6 && guard < 16; guard++)
                f.poll();
            if (std::strcmp(failure, "ERCRC") == 0)
                break;
            f.poll();
            f.poll();
        }
        replies["QPIRI"] = gkRating;
        replies["QPIWS"] = std::string(38, '0');
        f.cycles(2);
        assert(f.driver.protocol == PI30_PIP_GK && f.service.hasFreshBatterySample(testNow));
        assert(!f.snapshot()["LiveData"][DESCR_Battery_Discharge_Current].isNull());
        ++cases;
    }
    for (bool forced : {false, true})
        for (bool withQall : {false, true})
            for (bool missingRating : {false, true})
            {
                Fixture f(PI30_REVO);
                if (!forced)
                {
                    f.driver.protocol = PI30_UNKNOWN;
                    f.driver.forcedProtocol = NoD;
                }
                replies["QALL"] = withQall ? revoAll : "NAK";
                if (missingRating)
                    replies["QPIRI"] = "NAK";
                f.cycles(3);
                const auto snapshot = f.snapshot();
                // With no model evidence at all, retain the old no-QALL heuristic.
                const bool qallApplied = withQall && (forced || !missingRating);
                near(snapshot["LiveData"][DESCR_PV_Input_Power].as<double>(), 622);
                if (qallApplied)
                {
                    near(snapshot["LiveData"][DESCR_PV_Charging_Power].as<double>(), 1385);
                    near(snapshot["LiveData"][DESCR_PV1_Input_Current].as<double>(), 15.3);
                }
                assert(!snapshot["LiveData"][DESCR_PV2_Input_Power].isNull());
                assert(f.service.hasFreshBatterySample(testNow));
                ++cases;
            }
    std::printf("PASS: %u late-start/refinement cases, first committed flat/JSON data, "
                "integer/decimal/night replies, transport recovery and REVO with/without QALL.\n",
                cases);
}

static void exhaustStatic(Fixture &f)
{
    for (unsigned guard = 0; f.driver.requestStaticData && guard < 20; guard++)
        f.poll();
    assert(!f.driver.requestStaticData);
}

static void probeLateMax21()
{
    Fixture f(PI30_MAX);
    f.driver.protocol = PI30_UNKNOWN;
    f.driver.forcedProtocol = NoD;
    replies["QPIRI"] = "NAK";
    exhaustStatic(f);
    replies["QPIRI"] = maxRating;
    replies["QPIGS"] = fields(fields(fields(gkDay, 21, 9, "000"), 21, 15, "00007"), 21, 12, "0002");
    for (unsigned guard = 0; f.dynamicCompleted == 0 && guard < 30; guard++)
    {
        f.poll();
        const auto d = f.snapshot();
        const std::string raw = d["RawData"]["QPIGS"] | "";
        if (!raw.empty())
        {
            assert(!d["LiveData"][DESCR_Battery_Discharge_Current].isNull());
            near(d["LiveData"][DESCR_Battery_Discharge_Current].as<double>(), 7);
        }
    }
    assert(f.driver.protocol == PI30_MAX && f.dynamicCompleted == 1 &&
           f.driver.batterySampleSequence == 1);
}

static void probeDiscardThenMissing()
{
    Fixture f(PI30_PIP_GK);
    f.driver.protocol = PI30_UNKNOWN;
    f.driver.forcedProtocol = NoD;
    replies["QPIRI"] = "NAK";
    exhaustStatic(f);
    replies["QPIRI"] = gkRating;
    replies["QPIGS"] = fields(fields(gkDay, 21, 9, "000"), 21, 15, "00007");
    for (unsigned i = 0; i < 8; i++)
        f.poll();
    assert(f.dynamicCompleted == 0 && f.driver.protocol == PI30_PIP_GK);
    assert(!f.driver.haveBatterySample);
    replies["QPIGS"] = "NAK";
    f.cycles(1);
    const auto d = f.snapshot();
    assert(d["LiveData"][DESCR_PV_Generation_Day].isNull());
    assert(d["LiveData"][DESCR_Battery_Voltage].isNull());
    assert(!f.service.hasFreshBatterySample(testNow) && f.driver.batterySampleSequence == 0);
}

static void probePreviousFreshness()
{
    Fixture f(PI30_PIP_GK);
    f.driver.protocol = PI30_UNKNOWN;
    f.driver.forcedProtocol = NoD;
    replies["QPIRI"] = "NAK";
    exhaustStatic(f);
    f.cycles(1);
    assert(f.driver.protocol == PI30 && f.driver.haveBatterySample &&
           f.driver.batterySampleSequence == 1);
    const uint32_t seq = f.driver.batterySampleSequence, sampled = f.driver.lastBatterySampleAt,
                   accepted = f.driver.lastBatteryAcceptedAt;
    const auto completed = f.dynamicCompleted;
    std::string before;
    serializeJson(f.snapshot(), before);
    replies["QPIRI"] = gkRating;
    for (unsigned i = 0; i < 8; i++)
        f.poll();
    std::string after;
    serializeJson(f.snapshot(), after);
    assert(f.dynamicCompleted == completed && f.driver.protocol == PI30_PIP_GK);
    assert(f.driver.batterySampleSequence == seq && f.driver.lastBatterySampleAt == sampled &&
           f.driver.lastBatteryAcceptedAt == accepted);
    assert(f.driver.haveBatterySample && f.service.hasFreshBatterySample(testNow));
    assert(before == after);
}

static void probeLateQpiriCrc()
{
    Fixture f(PI30_PIP_GK);
    f.driver.protocol = PI30_UNKNOWN;
    f.driver.forcedProtocol = NoD;
    replies["QPIRI"] = "NAK";
    exhaustStatic(f);
    replies["QPIRI"] = "ERCRC";
    for (unsigned i = 0; i < 7; i++)
        f.poll();
    assert(f.driver.requestCounter == 0 && f.dynamicCompleted == 0);
    assert(!f.driver.haveBatterySample &&
           f.driver.cycleQpigs21Layout == PI_Serial::Qpigs21Layout::None);
    assert(std::string(f.snapshot()["RawData"]["QPIGS"] | "").empty());
    replies["QPIRI"] = gkRating;
    f.cycles(1);
    assert(f.driver.protocol == PI30_PIP_GK && f.driver.batterySampleSequence == 1);
}

static void startupRollbackRegressions()
{
    probeLateMax21();
    probeDiscardThenMissing();
    probePreviousFreshness();
    probeLateQpiriCrc();
    std::puts("PASS: late MAX/21, missing follow-up, preserved prior freshness and late QPIRI CRC "
              "regressions.");
}

static void modbusTimingDelegation()
{
    Fixture f(PI30_PIP_GK);
    MODBUS bus;
    f.driver.protocol = MODBUS_SMG;
    f.driver.modbus = &bus;
    bus.connection = true;
    bus.timing.available = true;
    bus.timing.sequence = 42;
    bus.timing.sampledAtMs = testNow;
    bus.timing.acceptedAtMs = testNow;
    bus.timing.validForMs = 155000;
    assert(!f.service.batterySampleTiming(testNow).available);
    f.driver.notifyStateUpdate(false);
    assert(bus.accepted == 0);
    f.driver.notifyStateUpdate(true);
    assert(bus.accepted == 1);
    assert(f.service.batterySampleTiming(testNow).sequence == 42);
    assert(f.service.hasFreshBatterySample(testNow + 150000));
    assert(!f.service.hasFreshBatterySample(testNow + 155001));
    f.driver.delayTime = 3200;
    testNow += 4000;
    f.driver.loop();
    assert(bus.interval == 3200);
    f.service._transportPaused = true;
    assert(!f.service.batterySampleTiming(testNow).available);
    f.service._transportPaused = false;
    bus.timing.available = false;
    assert(!f.service.batterySampleTiming(testNow).available);
    f.driver.modbus = nullptr;
    std::puts("PASS: Modbus producer timing and poll interval pass through PI/service; incomplete "
              "updates never accept a battery sample.");
}
