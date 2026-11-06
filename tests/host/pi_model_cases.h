#pragma once

static void legacyModelFallback()
{
    const char *qmd = "#####INVERTEX5K ###5000 99 1/1 230 230 04 12.0";
    for (auto protocol : {PI30, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (protocol != PI30_REVO && withoutQall)
                continue;
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                replies["QMN"] = "MKS2-11000";
                replies["QMD"] = qmd;
                f.cycles(2);
                const auto d = f.snapshot();
                assert(d["DeviceData"][DESCR_Device_Model] == "MKS2-11000");
                assert(std::count(requested.begin(), requested.end(), "QMD") == 0);
            }
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                replies["QMD"] = qmd;
                f.cycles(2);
                const auto d = f.snapshot();
                assert(d["DeviceData"][DESCR_Device_Model] == "INVERTEX5K");
                assert(d["RawData"]["QMN"] == "NAK" && d["RawData"]["QMD"] == qmd);
                assert(std::count(requested.begin(), requested.end(), "QMD") == 1);
                // The QMD rating is generic; the full QPIRI remains authoritative.
                if (protocol != PI30_MAX)
                {
                    near(d["DeviceData"][DESCR_AC_Out_Rating_Active_Power].as<double>(), 4000);
                    near(d["DeviceData"][DESCR_Battery_Rating_Voltage].as<double>(), 48);
                }
                bool published = false;
                for (const auto &m : f.mqtt._mqtt.messages)
                    if (m.topic == "solar/test/DeviceData/Device_Model" && m.value == "INVERTEX5K")
                        published = true;
                assert(published);
            }
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                replies["QMD"] = sanitizePiReplyText("     INVERTEX5K    5000 99 1/1 230 230 04 12.0").text;
                f.cycles(2);
                assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "INVERTEX5K");
            }
        }
    for (const char *answer : {"NAK", "NOA", "ERCRC", "", "bad", "INVERTEX5K ###5000",
                               "############### ###5000 99 1/1 230 230 04 12.0",
                               "#####INVER#EX5K ###5000 99 1/1 230 230 04 12.0",
                               "#####INVERTEX5K!###5000 99 1/1 230 230 04 12.0",
                               "#####INVER\nEX5K ###5000 99 1/1 230 230 04 12.0"})
    {
        Fixture f(PI30_PIP_GK);
        replies["QMD"] = answer;
        const bool accepted = f.driver.PIXX_QMN();
        assert(accepted);
        assert(staticData[DESCR_Device_Model].isNull());
    }
    // Padding on either side is removed without changing the model spelling.
    for (const char *model : {"     INVERTEX5K", "INVERTEX5K     ", "###INVERTEX5K##"})
    {
        std::string padded(model);
        padded.resize(15, ' ');
        assert(pi_qmd_model((padded + " ###5000 99 1/1 230 230 04 12.0").c_str()) == "INVERTEX5K");
        // Compile the actual transport normalization, which collapses spaces
        // before this parser sees a successful UART response.
        assert(pi_qmd_model(sanitizePiReplyText((padded + "    5000 99 1/1 230 230 04 12.0").c_str())) == "INVERTEX5K");
    }
    for (const char *answer : {"NOA", "ERCRC", "", "bad"})
    {
        Fixture f(PI30_PIP_GK);
        replies["QMN"] = answer;
        replies["QMD"] = qmd;
        assert(f.driver.PIXX_QMN() == (std::strcmp(answer, "ERCRC") != 0));
        assert(std::count(requested.begin(), requested.end(), "QMD") == 0);
    }
    assert(pi_qmd_model(sanitizePiReplyText("     INVERTER 5K    5000 99 1/1 230 230 04 12.0")) == "INVERTER 5K");
    for (const char *malformed : {"INVERTEX5K garbage", "INVERTEX5K ####### 99 1/1 230 230 04 12.0",
                                 "INVERTEX5K ###5000 99 111 230 230 04 12.0",
                                 "INVERTEX5K ###5000 99 1/1 230 230 04 1200",
                                 "INVERTEX5K ###5000 99 1/1 230 230 04 12.0 extra",
                                 "ABCDEFGHIJKLMNOP ###5000 99 1/1 230 230 04 12.0"})
        assert(pi_qmd_model(malformed).isEmpty());
    // The original QMN CRC handling still rolls back the static cycle.
    {
        Fixture f(PI30_PIP_GK);
        replies["QMN"] = "old-model";
        f.cycles(2);
        f.driver.requestStaticData = true;
        f.driver.requestCounter = 0;
        replies["QMN"] = "ERCRC";
        f.poll(); // QPIRI
        f.poll(); // QMN CRC failure
        assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "old-model");
        assert(staticData[DESCR_Device_Model] == "old-model");
        assert(f.driver.requestCounter == 0);
        assert(std::count(requested.begin(), requested.end(), "QMD") == 0);
        replies["QMN"] = "NAK";
        replies["QMD"] = qmd;
        f.cycles(2);
        assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "INVERTEX5K");
    }
    std::puts("PASS: QMN priority, validated legacy QMD model only, original QPIRI ratings, "
              "MAX/GK/PI41/REVO first cycles with/without QALL, MQTT, failures and rollback.");
}

static void legacyModelFailures()
{
    const char *qmd = "#####INVERTEX5K ###5000 99 1/1 230 230 04 12.0";
    for (auto protocol : {PI30, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
        for (bool withoutQall : {false, true})
        {
            if (protocol != PI30_REVO && withoutQall)
                continue;
            for (const char *failure : {"ERCRC", "NAK", "NOA", "bad"})
            {
                Fixture f(protocol);
                if (withoutQall)
                    replies["QALL"] = "NAK";
                replies["QMD"] = failure;
                for (unsigned i = 0; i < 100; ++i)
                    f.poll();
                assert(!f.driver.requestStaticData && f.driver.haveBatterySample);
                assert(f.dynamicCompleted >= 2 && f.service.hasFreshBatterySample(testNow));
                assert(std::count(requested.begin(), requested.end(), "QMD") == 1);
                assert(f.snapshot()["DeviceData"][DESCR_Device_Model].isNull());
                assert(f.snapshot()["RawData"]["QMD"] == failure);

                // Recover on the next explicitly requested static refresh.
                // Optional metadata must not be retried before every measurement.
                f.cycles(1);
                auto sequence = f.driver.batterySampleSequence;
                replies["QMD"] = qmd;
                f.driver.requestStaticData = true;
                f.driver.requestCounter = 0;
                f.cycles(1);
                assert(f.driver.batterySampleSequence > sequence);
                assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "INVERTEX5K");
                assert(std::count(requested.begin(), requested.end(), "QMD") == 2);

                // A later failure preserves previously validated metadata and
                // remains visible in RawData while normal measurements continue.
                sequence = f.driver.batterySampleSequence;
                replies["QMD"] = "ERCRC";
                f.driver.requestStaticData = true;
                f.driver.requestCounter = 0;
                f.cycles(1);
                assert(f.driver.batterySampleSequence > sequence);
                assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "INVERTEX5K");
                assert(f.snapshot()["RawData"]["QMD"] == "ERCRC");
                assert(std::count(requested.begin(), requested.end(), "QMD") == 3);

                replies["QMN"] = "preferred-model";
                f.driver.requestStaticData = true;
                f.driver.requestCounter = 0;
                f.cycles(1);
                assert(f.snapshot()["DeviceData"][DESCR_Device_Model] == "preferred-model");
                assert(std::count(requested.begin(), requested.end(), "QMD") == 3);
            }
        }
    std::puts("PASS: permanent optional QMD CRC/NAK/timeout/malformed replies cannot block "
              "MAX/GK/PI41/REVO startup; bounded attempts, recovery and valid model retention.");
}
