#pragma once

static void completeSnapshots()
{
    for (auto protocol : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
    {
        Fixture f(protocol);
        whileWaiting = [&](const std::string &) { f.checkSnapshot(); };
        f.cycles(4);
        whileWaiting = nullptr;
        assert(f.snapshotChecks > 60);
        const auto final = f.snapshot();
        if (protocol == PI30_REVO)
        {
            const char *fault = final["LiveData"][DESCR_Fault_Code].as<const char *>();
            const char *warning = final["LiveData"][DESCR_Warning_Code].as<const char *>();
            assert(fault && warning);
            assert(std::strcmp(fault, "Inverter overvoltage") == 0);
            assert(std::strcmp(warning, "Low battery") == 0);
        }
        // MQTT JSON mode and flat HA publications must use the same completed data.
        _settings.get.json = true;
        f.mqtt._mqtt.messages.clear();
        f.mqtt.publishState();
        bool json = false;
        for (const auto &p : f.mqtt._mqtt.messages)
            if (p.topic == "solar/test/Data")
            {
                JsonDocument decoded;
                assert(!deserializeJson(decoded, p.value));
                assert(!decoded["LiveData"][DESCR_PV2_Input_Power].isNull());
                json = true;
            }
        assert(json);
    }
    std::puts("PASS: real PI scheduler, parsers, completed snapshots and flat/JSON MQTT for MAX, "
              "GK, PI41 and REVO with QPIGS/QALL, including every UART wait and poll step.");
}

static void ownershipAndRecovery()
{
    for (auto protocol : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
    {
        Fixture f(protocol);
        f.cycles(2);
        const double voltage = liveData[DESCR_PV2_Input_Voltage].as<double>();
        const double power = liveData[DESCR_PV2_Input_Power].as<double>();
        const String fault = liveData[DESCR_Fault_Code].as<const char *>();
        assert(f.driver.PIXX_QPIGS());
        near(liveData[DESCR_PV2_Input_Voltage].as<double>(), voltage);
        near(liveData[DESCR_PV2_Input_Power].as<double>(), power);
        if (protocol != PI30_REVO)
            assert(std::strcmp(liveData[DESCR_Fault_Code].as<const char *>(), fault.c_str()) == 0);
        replies["QPIGS2"] = "ERCRC";
        assert(!f.driver.PIXX_QPIGS2());
        near(liveData[DESCR_PV2_Input_Power].as<double>(), power);
        replies["QPIGS2"] = protocol == PI30_PIP_GK ? legacyPv2
                            : protocol == PI41      ? pi41Pv2
                                                    : "01.9 388.9 00745";
        f.driver.requestCounter = 0;
        f.driver.requestStaticData = false;
        f.driver.get.raw.qpigs2 = replies["QPIGS2"].c_str();
        f.driver.beginCycleBackup();
        liveData[DESCR_Battery_Charge_Energy] = 12.5;
        liveData["DS18B20_0"] = 24.25;
        f.driver.get.raw.qpigs = "partial";
        liveData[DESCR_PV2_Input_Power] = 999;
        f.driver.restoreCycleBackup();
        near(liveData[DESCR_Battery_Charge_Energy].as<double>(), 12.5);
        near(liveData["DS18B20_0"].as<double>(), 24.25);
        near(liveData[DESCR_PV2_Input_Power].as<double>(), power);
        assert(f.driver.get.raw.qpigs != "partial");
        f.checkSnapshot();
    }
    std::puts("PASS: field ownership, CRC preservation, raw/live rollback and independent "
              "energy/temperature updates.");
}

static void changingMeasurements()
{
    for (auto protocol : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
    {
        Fixture f(protocol);
        f.cycles(2);
        auto old = f.snapshot();
        const double oldPower = old["LiveData"][DESCR_PV2_Input_Power].as<double>();
        const std::string oldRaw = old["RawData"]["QPIGS2"].as<const char *>();
        const char *next =
            protocol == PI30_PIP_GK
                ? "0002 084.8 46.80 00112 10000000 0000 0000 0000 000.0 00.00 00000 00215"
            : protocol == PI41 ? "230.0 50.0 230.0 50.0 1000 0900 020 0007 090.0 47.20 00100100"
                               : "02.0 400.0 00810";
        replies["QPIGS2"] = next;
        f.poll();
        f.poll();
        assert(liveData[DESCR_PV2_Input_Power].as<double>() != oldPower);
        auto during = f.snapshot();
        near(during["LiveData"][DESCR_PV2_Input_Power].as<double>(), oldPower);
        assert(std::strcmp(during["RawData"]["QPIGS2"].as<const char *>(), oldRaw.c_str()) == 0);
        f.cycles(1);
        auto fresh = f.snapshot();
        assert(std::strcmp(fresh["RawData"]["QPIGS2"].as<const char *>(), next) == 0);
        near(fresh["LiveData"][DESCR_PV2_Input_Power].as<double>(), protocol == PI30_PIP_GK ? 112
                                                                    : protocol == PI41      ? 630
                                                                                            : 800);
    }
    std::puts("PASS: changing PV measurements remain paired with their own raw reply until the "
              "whole new cycle is committed.");
}

static void errorsAndCommands()
{
    for (auto protocol : {PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
    {
        Fixture f(protocol);
        f.cycles(2);
        const auto oldCompleted = f.completed;
        // Stop late in a cycle; the already published snapshot must remain usable.
        f.poll();
        f.poll();
        f.poll();
        replies["QPIWS"] = "ERCRC";
        for (unsigned i = 0; i < 4; i++)
            f.poll();
        assert(f.completed == oldCompleted);
        f.checkSnapshot();
        replies["QPIWS"] = protocol == PI30_REVO ? "NAK" : std::string(38, '0');
        f.cycles(1);
        const auto beforeCommand = f.completed;
        f.poll();
        f.poll();
        replies["TEST-WRITE"] = "ACK-test";
        f.driver.sendCommand("TEST-WRITE");
        f.poll();
        assert(f.completed == beforeCommand);
        bool answer = false;
        for (const auto &m : f.mqtt._mqtt.messages)
            if (m.topic == "solar/test/DeviceControl/Set_Command_answer" && m.value == "ACK-test")
                answer = true;
        assert(answer);
        f.cycles(1);
    }
    Fixture f(PI30_PIP_GK);
    f.cycles(1);
    f.state.resetInverterState();
    auto empty = f.snapshot();
    assert(empty["LiveData"][DESCR_PV2_Input_Power].isNull());
    liveData["DS18B20_0"] = 25.5;
    liveData[DESCR_Battery_Charge_Energy] = 3.5;
    f.state.commitLocalState();
    auto local = f.snapshot();
    near(local["LiveData"]["DS18B20_0"].as<double>(), 25.5);
    near(local["LiveData"][DESCR_Battery_Charge_Energy].as<double>(), 3.5);
    std::puts("PASS: late CRC abort, recovery, manual command during a cycle, snapshot reset and "
              "independent local data.");
}

static void concurrentSnapshots()
{
    SolarState state;
    state.begin();
    std::atomic_bool running{true};
    std::atomic_uint reads{0};
    std::thread reader(
        [&]
        {
            while (running)
            {
                JsonDocument d;
                state.snapshotTo(d);
                if (!d["RawData"]["QPIGS2"].isNull())
                {
                    const unsigned raw =
                        std::strtoul(d["RawData"]["QPIGS2"].as<const char *>(), nullptr, 10);
                    assert(d["LiveData"][DESCR_PV2_Input_Power].as<unsigned>() == raw);
                }
                ++reads;
            }
        });
    for (unsigned i = 1; i <= 2000; i++)
    {
        liveData[DESCR_PV2_Input_Power] = i;
        state.updateRaw("QPIGS2", String(static_cast<unsigned long>(i)));
        state.commitInverterState();
    }
    running = false;
    reader.join();
    assert(reads > 0);
    std::puts(
        "PASS: concurrent HTTP-style readers never observe a partial committed inverter snapshot.");
}
