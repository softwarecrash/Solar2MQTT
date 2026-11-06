#pragma once

namespace custom_query_cases
{
static unsigned sent(const char *command)
{
    return static_cast<unsigned>(
        std::count(requested.begin(), requested.end(), std::string(command)));
}
static double acceptedPower(Fixture &f)
{
    const auto d = f.snapshot();
    const auto l = d["LiveData"];
    return l[DESCR_Battery_Voltage].template as<double>() *
           (l[DESCR_Battery_Charge_Current].template as<double>() -
            l[DESCR_Battery_Discharge_Current].template as<double>());
}

static void inquiryNames()
{
    for (const char *q :
         {"QPI",       "QPIRI",       "QMN",     "QGMN",      "QMD",        "QPIBI",
          "QID",       "QSID",        "QVFW",    "QVFW2",     "QVFW3",      "QSVFW2",
          "QFLAG",     "QMOD",        "QPIWS",   "QPIGS",     "QPIGS2",     "QALL",
          "Q1",        "QT",          "QET",     "QLT",       "QBEQI",      "QEY2026",
          "QVFW4",     "QDI",         "QMCHGCR", "QMUCHGCR", "QMSCHGCR",   "QOPM",
          "QCST",      "QCVT",        "QFET",
          "QEM202611", "QED20261105", "QLY2026", "QLM202611", "QLD20261105"})
        assert(PI_Serial::isReadOnlyPiQuery(q));
    for (const char *q : {"", "Q", "q1", "QBOGUS", "Q1 extra", "QVFW2 extra", "QED20261105X",
                          "QED2026", "QED2026110X", "QEM20261", "QEY20260", "QT\nPBT00", "PEa",
                          "PBT00", "DAT20261105120000", "powmr pi Q1", "QVFW40",
                          "QMSCHGCR080", "QCST00", "QCVT255", "QOPM01", "QDI extra", "QFET0"})
        assert(!PI_Serial::isReadOnlyPiQuery(q));
    std::puts("PASS: explicit inquiry names/date suffixes; settings, unknown Q commands and "
              "appended text retain the legacy path.");
}

static void firstCycleAndBoundaries()
{
    for (auto p : {PI16, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (p != PI30_REVO && withoutQall)
                continue;
            Fixture f(p);
            if (withoutQall)
                replies["QALL"] = "NAK";
            replies["QVFW"] = "VERFW:00075.20";
            f.driver.sendCommand("QVFW");
            f.cycles(1);
            assert(sent("QVFW") == 0);
            assert(f.driver.haveBatterySample);
            const uint32_t seq = f.driver.batterySampleSequence,
                           accepted = f.driver.lastBatteryAcceptedAt,
                           sampled = f.driver.lastBatterySampleAt,
                           lease = f.driver.lastBatteryValidForMs;
            const auto complete = f.dynamicCompleted;
            f.poll();
            assert(sent("QVFW") == 1 && f.driver.lastCustomCommandWasQuery);
            assert(f.dynamicCompleted == complete && !f.driver.requestStaticData &&
                   f.driver.requestCounter == 0 && !f.driver.cycleBackupActive);
            assert(f.driver.batterySampleSequence == seq &&
                   f.driver.lastBatteryAcceptedAt == accepted &&
                   f.driver.lastBatterySampleAt == sampled &&
                   f.driver.lastBatteryValidForMs == lease);
            bool answer = false;
            for (const auto &m : f.mqtt._mqtt.messages)
                if (m.topic == "solar/test/DeviceControl/Set_Command_answer" &&
                    m.value == "VERFW:00075.20")
                    answer = true;
            assert(answer);
            // A second inquiry must wait for the next full normal measurement.
            f.driver.sendCommand("QVFW");
            f.poll();
            assert(sent("QVFW") == 1 && f.driver.cycleBackupActive);
            f.cycles(1);
            assert(sent("QVFW") == 1);
            f.poll();
            assert(sent("QVFW") == 2);
            // Even an available slot cannot interrupt a partially parsed cycle.
            f.cycles(1);
            f.poll();
            assert(f.driver.cycleBackupActive);
            const auto step = f.driver.requestCounter;
            f.driver.sendCommand("QVFW");
            f.poll();
            assert(sent("QVFW") == 2 && f.driver.requestCounter == step + 1);
            f.cycles(1);
            f.poll();
            assert(sent("QVFW") == 3);
        }
    std::puts("PASS: PI16/MAX/GK/PI41/REVO with/without QALL: first measurement precedes a query; "
              "one query per full cycle, unchanged snapshot/timing and MQTT reply.");
}

static void repeatedQueriesAndEnergy()
{
    for (auto p : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (p != PI30_REVO && withoutQall)
                continue;
            for (const char *answer : {"VERFW:00075.20", "NAK", "NOA", "ERCRC"})
                for (uint32_t phase : {0u, UINT32_MAX - 25000u})
                {
                    Fixture f(p);
                    if (withoutQall)
                        replies["QALL"] = "NAK";
                    testNow += phase;
                    replies["QPIGS"] =
                        fields(fields(fields(replies["QPIGS"], p == PI30_MAX ? 24 : 21, 8, "50.00"),
                                      p == PI30_MAX ? 24 : 21, 9, "010"),
                               p == PI30_MAX ? 24 : 21, 15, "00000");
                    f.cycles(2);
                    replies["QVFW"] = answer;
                    replyDelays["QVFW"] = 1900;
                    const double watts = acceptedPower(f);
                    assert(watts != 0);
                    EnergyBacklog energy;
                    energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow),
                                               testNow);
                    const auto start = testNow;
                    unsigned previousSequence = 0, executions = 0;
                    whileWaiting = [&](const std::string &command)
                    {
                        if (command != "QVFW")
                            return;
                        assert(!f.driver.requestStaticData && f.driver.requestCounter == 0 &&
                               !f.driver.cycleBackupActive);
                        assert(f.driver.batterySampleSequence > previousSequence);
                        previousSequence = f.driver.batterySampleSequence;
                        ++executions;
                        f.driver.sendCommand("QVFW"); // a new request during UART must survive
                    };
                    f.driver.sendCommand("QVFW");
                    for (unsigned i = 0; i < 120; i++)
                    {
                        f.poll();
                        assert(f.service.hasFreshBatterySample(testNow));
                        energy.updateBatteryEnergy(f.state, f.service.batterySampleTiming(testNow),
                                                   testNow);
                    }
                    whileWaiting = nullptr;
                    assert(executions >= 10);
                    assert(!f.driver.requestStaticData);
                    const double expected =
                        std::fabs(watts) * uint32_t(testNow - start) / 3600000.0;
                    near(watts > 0 ? energy._batteryChargeWh : energy._batteryDischargeWh,
                         expected);
                }
        }
    std::puts("PASS: sustained 1.9-second queries, good/NAK/no-answer/CRC replies, exact energy "
              "integral and fresh samples across MAX/GK/PI41/REVO variants, including timer wrap.");
}

static void statusQueriesStayDiagnostic()
{
    for (auto p : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (const char *query : {"QPIGS", "QPIGS2", "QALL"})
        {
            Fixture f(p);
            f.cycles(2);
            const auto before = f.snapshot();
            std::string liveBefore;
            serializeJson(before["LiveData"], liveBefore);
            const uint32_t sequence = f.driver.batterySampleSequence,
                           sampled = f.driver.lastBatterySampleAt;
            // Deliberately different raw data: a manual response is diagnostic,
            // not a substitute for the next complete scheduled measurement.
            replies[query] = "123 456 789";
            f.driver.sendCommand(query);
            f.poll();
            const auto after = f.snapshot();
            std::string liveAfter;
            serializeJson(after["LiveData"], liveAfter);
            assert(liveBefore == liveAfter && f.driver.batterySampleSequence == sequence &&
                   f.driver.lastBatterySampleAt == sampled);
            bool answer = false;
            for (const auto &message : f.mqtt._mqtt.messages)
                if (message.topic == "solar/test/DeviceControl/Set_Command_answer" &&
                    message.value == "123 456 789")
                    answer = true;
            assert(answer && !f.driver.requestStaticData);
        }
    std::puts("PASS: manual QPIGS/QPIGS2/QALL responses cannot change the published measurement or "
              "renew its battery timestamp.");
}

static void failuresAndLegacyCommands()
{
    Fixture f(PI30_PIP_GK);
    f.cycles(2);
    replies["QVFW"] = "VERFW:00075.20";
    // Query traffic never hides an actually expired reading.
    f.driver.sendCommand("QVFW");
    replyDelays["QVFW"] = 30000;
    f.poll();
    assert(!f.service.hasFreshBatterySample(testNow));
    replyDelays.clear();
    f.cycles(1);
    assert(f.service.hasFreshBatterySample(testNow));
    // A corrupt normal cycle cannot be replaced by a version reply.
    f.driver.sendCommand("QVFW");
    f.poll(); // consume the preceding cycle's slot
    replies["QPIGS"] = "ERCRC";
    f.driver.sendCommand("QVFW");
    const auto count = sent("QVFW"), seq = f.driver.batterySampleSequence;
    for (unsigned i = 0; i < 30; i++)
        f.poll();
    // One inquiry may follow the failed attempt; it cannot replace a measurement.
    assert(sent("QVFW") == count + 1 && f.driver.batterySampleSequence == seq);
    assert(!f.service.hasFreshBatterySample(testNow));
    replies["QPIGS"] = gkDay;
    f.cycles(1);
    f.poll();
    assert(sent("QVFW") == count + 1);
    // Classification remains paired to the in-flight inquiry; an incoming
    // setting request waits in the protected buffer and still triggers refresh.
    f.cycles(1);
    replies["PEa"] = "ACK";
    bool queued = false;
    whileWaiting = [&](const std::string &command)
    {
        if (command == "QVFW" && !queued)
        {
            queued = true;
            std::thread producer([&] { f.driver.sendCommand("PEa"); });
            producer.join();
        }
    };
    f.driver.sendCommand("QVFW");
    f.poll();
    whileWaiting = nullptr;
    assert(queued && !f.driver.requestStaticData && f.driver.customCommandBuffer == "PEa");
    f.poll();
    assert(sent("PEa") == 1 && !f.driver.lastCustomCommandWasQuery && f.driver.requestStaticData);
    f.cycles(1);
    replies["QUNKNOWN"] = "NAK";
    f.driver.sendCommand("QUNKNOWN");
    f.poll();
    assert(f.driver.requestStaticData && !f.driver.lastCustomCommandWasQuery);
    // Native Modbus and explicit PowMr PI commands retain their existing path.
    for (auto p : {MODBUS_SMG, MODBUS_POWMR_PI})
    {
        Fixture m(PI30_PIP_GK);
        MODBUS bus;
        m.driver.protocol = p;
        m.driver.modbus = &bus;
        m.driver.sendCommand("QVFW");
        m.poll();
        assert(bus.lastCommand == "QVFW" && !m.driver.lastCustomCommandWasQuery);
        m.driver.sendCommand(p == MODBUS_SMG ? "read 215" : "powmr pi QVFW");
        replies["QVFW"] = "VERFW:test";
        m.poll();
        assert(!m.driver.lastCustomCommandWasQuery && m.driver.requestStaticData);
        if (p == MODBUS_SMG)
            assert(bus.lastCommand == "read 215");
        else
            assert(sent("QVFW") == 1 && m.driver.protocol == p && m.driver.serialIntfBaud == 2400);
        m.driver.modbus = nullptr;
    }
    failNextSemaphoreCreate() = true;
    PI_Serial unavailable(testUart, 1, 2);
    assert(unavailable.customCommandMutex == nullptr);
    assert(unavailable.sendCommand("QVFW") == "");
    assert(!unavailable.sendCustomCommand());
    std::puts("PASS: real expiry, normal-cycle CRC/recovery, concurrent incoming command, "
              "unchanged setting/unknown/Modbus/hybrid handling and allocation failure.");
}

static void failedPollingStillAllowsDiagnosis()
{
    for (auto protocol : {PI16, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (protocol != PI30_REVO && withoutQall)
                continue;
            // A failed initial static read must still allow a diagnostic query.
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                replies["QPIRI"] = "ERCRC";
                replies["QVFW"] = "VERFW:00075.20";
                f.driver.sendCommand("QVFW");
                f.poll();
                assert(sent("QVFW") == 0 && sent("QPIRI") == 1);
                f.poll();
                assert(sent("QVFW") == 1);
                assert(f.driver.requestStaticData && f.driver.requestCounter == 0);
                assert(!f.driver.haveBatterySample && f.driver.batterySampleSequence == 0);
                assert(!f.driver.haveDynamicReply);
            }
            for (uint32_t phase : {0u, UINT32_MAX - 30000u})
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                testNow += phase;
                f.cycles(2);
                replies["QVFW"] = "VERFW:00075.20";
                f.driver.sendCommand("QVFW");
                f.poll(); // Consume the preceding successful cycle's slot.
                const auto sequence = f.driver.batterySampleSequence;
                const auto accepted = f.driver.lastBatteryAcceptedAt;
                const auto sampled = f.driver.lastBatterySampleAt;
                const auto status = replies["QPIGS"];
                const auto qall = replies["QALL"];
                replies["QALL"] = "NAK";
                replies["QPIGS"] = "ERCRC";
                for (unsigned attempt = 0; attempt < 5; ++attempt)
                {
                    const auto previousReads = sent("QPIGS");
                    const auto previousQueries = sent("QVFW");
                    f.driver.sendCommand("QVFW");
                    f.poll();
                    assert(sent("QPIGS") == previousReads + 1);
                    assert(sent("QVFW") == previousQueries);
                    const auto lastMeasurementReply = f.driver.lastDynamicReplyAt;
                    f.poll();
                    assert(sent("QVFW") == previousQueries + 1);
                    assert(f.driver.lastDynamicReplyAt == lastMeasurementReply);
                    assert(!f.driver.requestStaticData && f.driver.requestCounter == 0);
                    assert(f.driver.batterySampleSequence == sequence &&
                           f.driver.lastBatteryAcceptedAt == accepted &&
                           f.driver.lastBatterySampleAt == sampled);
                }
                // Diagnosis must not refresh expired battery data or its energy lease.
                testNow += f.driver.lastBatteryValidForMs + 1;
                assert(!f.service.hasFreshBatterySample(testNow));
                f.driver.sendCommand("QVFW");
                f.poll();
                f.poll();
                assert(!f.driver.connection);
                replies["QALL"] = qall;
                replies["QPIGS"] = status;
                f.cycles(1);
                assert(f.driver.batterySampleSequence > sequence);
                assert(f.service.hasFreshBatterySample(testNow));
            }
        }
    std::puts("PASS: initial and repeated failed polling attempts still permit one diagnostic "
              "query, keep measurements stale, resume polling, and tolerate timer wrap.");
}

static void legacyInquiries()
{
    const char *const queries[] = {"QVFW4", "QDI", "QMCHGCR", "QMUCHGCR", "QMSCHGCR",
                                   "QOPM", "QCST", "QCVT", "QFET"};
    for (auto p : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (p != PI30_REVO && withoutQall)
                continue;
            for (const char *response : {"000", "NAK", "NOA", "ERCRC"})
            {
                Fixture f(p);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                f.driver.sendCommand(queries[0]);
                replies[queries[0]] = response;
                f.cycles(1);
                assert(sent(queries[0]) == 0);
                for (const char *query : queries)
                {
                    replies[query] = response;
                    f.driver.sendCommand(query);
                    const auto seq = f.driver.batterySampleSequence;
                    const auto completed = f.dynamicCompleted;
                    f.poll();
                    assert(sent(query) == 1 && f.driver.lastCustomCommandWasQuery);
                    assert(!f.driver.requestStaticData && f.dynamicCompleted == completed &&
                           f.driver.batterySampleSequence == seq);
                    f.cycles(1);
                    assert(f.driver.batterySampleSequence > seq &&
                           f.service.hasFreshBatterySample(testNow));
                }
            }
        }
    std::puts("PASS: all nine legacy inquiries use the protected queue on MAX/GK/PI41/REVO "
              "with/without QALL, including first startup, rejection, timeout and CRC.");
}
} // namespace custom_query_cases

static void customQueryCases()
{
    custom_query_cases::inquiryNames();
    custom_query_cases::firstCycleAndBoundaries();
    custom_query_cases::repeatedQueriesAndEnergy();
    custom_query_cases::statusQueriesStayDiagnostic();
    custom_query_cases::failuresAndLegacyCommands();
    custom_query_cases::legacyInquiries();
    custom_query_cases::failedPollingStillAllowsDiagnosis();
}
