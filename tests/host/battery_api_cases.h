#pragma once
#include "battery_test_helpers.h"

namespace battery_cases
{
static JsonDocument request()
{
    JsonDocument b;
    b["schema"] = 1;
    b["unit"] = "kWh";
    b["requestId"] = id;
    b["operation"] = "restore";
    b["historyMode"] = "carry_visible_baseline";
    b["deviceId"] = "test-board";
    b["bootId"] = "test-boot";
    b["chargeKwh"] = 1.25;
    b["dischargeKwh"] = 2.5;
    return b;
}
static void apiAndMqtt()
{
    reset(false);
    Fixture f(PI30_PIP_GK);
    f.mqtt._energyBacklog.begin();
    f.mqtt.initializeBatteryApi("test-board", "test-boot");
    f.mqtt._energyBacklog.updateBatteryEnergy(f.state, BatterySampleTiming{}, testNow);
    f.mqtt._mqtt.messages.clear();
    f.mqtt.publishState();
    assert(f.publication(DESCR_Battery_Charge_Energy) == "<absent>");
    std::vector<String> topics;
    f.mqtt.preserveBatteryDiscoveryTopics(topics);
    assert(topics.size() == 2);
    f.mqtt._haDiscoveryTopics = topics;
    f.mqtt._mqtt.messages.clear();
    f.mqtt.publishHaDiscovery(true);
    for (const auto &m : f.mqtt._mqtt.messages)
        for (const auto &topic : topics)
            assert(m.topic != topic.text || !m.value.empty());
    JsonDocument response;
    auto b = request();
    b["deviceId"] = "other-board";
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    b = request();
    b["bootId"] = "old-boot";
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    for (const char *key :
         {"schema", "unit", "requestId", "chargeKwh", "dischargeKwh", "operation", "historyMode"})
    {
        b = request();
        b.remove(key);
        assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 400);
    }
    for (double v : {-1.0, 1.0e10, std::nan(""), std::numeric_limits<double>::infinity()})
    {
        b = request();
        b["chargeKwh"] = v;
        assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 400);
    }
    b = request();
    b["chargeKwh"] = true;
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 400);
    b = request();
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
    b["chargeKwh"] = 9;
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    b = request();
    b["requestId"] = otherId;
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    f.mqtt.processBatteryMigration(testNow);
    JsonDocument diag;
    f.mqtt.copyBatteryDiagnostics(diag);
    assert(diag["job"]["state"] == "applied" && diag["migration"]["pending"] == false);
    near(diag["chargeKwh"].as<double>(), 1.25);
    f.mqtt._mqtt.messages.clear();
    f.mqtt.publishState();
    assert(f.publication(DESCR_Battery_Charge_Energy) == "1.25");
    assert(f.publication(DESCR_Battery_Discharge_Energy) == "2.5");
    b = request();
    b["bootId"] = "old-boot";
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 200);
    assert(response["state"] == "already_applied");
    b["requestId"] = otherId;
    assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    std::atomic<unsigned> reads{0};
    // Bound both workers; an unbounded reader can repeatedly reacquire the mutex.
    std::thread reader(
        [&]
        {
            for (unsigned i = 0; i < 100; i++)
            {
                JsonDocument copy;
                f.mqtt.copyBatteryDiagnostics(copy);
                assert(copy["deviceId"] == "test-board");
                ++reads;
            }
        });
    for (unsigned i = 0; i < 100; i++)
    {
        testNow += 1000;
        f.mqtt.refreshBatteryDiagnostics(testNow, true);
    }
    reader.join();
    assert(reads.load() == 100);
    reset(false);
    storage.blobs["batenergy:counter_a"] = {1, 2, 3};
    Fixture recovery(PI30_PIP_GK);
    recovery.mqtt._energyBacklog.begin();
    recovery.mqtt.initializeBatteryApi("test-board", "test-boot");
    b = request();
    assert(recovery.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 409);
    b["acknowledgeRecovery"] = true;
    assert(recovery.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
    recovery.mqtt.processBatteryMigration(testNow);
    assert(!recovery.mqtt._energyBacklog.batteryMigrationPending());
    reset(false);
    Fixture zero(PI30_PIP_GK);
    zero.mqtt._energyBacklog.begin();
    zero.mqtt.initializeBatteryApi("test-board", "test-boot");
    counters(zero.mqtt._energyBacklog, 10, 20);
    zero.mqtt.refreshBatteryDiagnostics(testNow, true);
    b = request();
    b["operation"] = "start_zero";
    b["historyMode"] = "start_new_series";
    assert(zero.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 400);
    b["chargeKwh"] = 0;
    b["dischargeKwh"] = 0;
    assert(zero.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
    zero.mqtt.processBatteryMigration(testNow);
    assert(!zero.mqtt._energyBacklog.batteryMigrationPending());
    near(zero.mqtt._energyBacklog._batteryChargeWh, 10);
    near(zero.mqtt._energyBacklog._batteryDischargeWh, 20);
    assert(zero.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 200);
    zero.mqtt.processBatteryMigration(testNow);
    near(zero.mqtt._energyBacklog._batteryChargeWh, 10);
    std::puts("PASS: main-loop migration queue, identity/schema/recovery checks, concurrent "
              "diagnostics and unchanged MQTT/HA discovery identities.");
}
static void discoveryAfterMigration()
{
    for (bool zero : {false, true})
        for (bool reconnect : {false, true})
        {
            reset(false);
            Fixture f(PI30_PIP_GK);
            f.mqtt._energyBacklog.begin();
            f.mqtt.initializeBatteryApi("test-board", "test-boot");
            f.mqtt._energyBacklog.updateBatteryEnergy(f.state, BatterySampleTiming{}, testNow);
            f.mqtt.publishHaDiscovery(true);
            auto countConfigs = [&]
            {
                unsigned count = 0;
                for (const auto &m : f.mqtt._mqtt.messages)
                {
                    if (m.topic.find("homeassistant/sensor/") != 0)
                        continue;
                    if (m.topic.find("/Battery_Charge_Energy/config") == std::string::npos &&
                        m.topic.find("/Battery_Discharge_Energy/config") == std::string::npos)
                        continue;
                    assert(!m.value.empty());
                    JsonDocument decoded;
                    assert(!deserializeJson(decoded, m.value));
                    const std::string stateTopic = decoded["state_topic"] | "";
                    assert(stateTopic == "solar/test/LiveData/Battery_Charge_Energy" ||
                           stateTopic == "solar/test/LiveData/Battery_Discharge_Energy");
                    const std::string key = stateTopic.substr(stateTopic.find_last_of('/') + 1);
                    assert(std::string(decoded["unique_id"] | "") ==
                           std::string(getHaDeviceId().c_str()) + ".LiveData." + key);
                    ++count;
                }
                return count;
            };
            assert(countConfigs() ==
                   0); // Pending migration must retain old config, without making a fake zero.
            auto b = request();
            if (zero)
            {
                b["operation"] = "start_zero";
                b["historyMode"] = "start_new_series";
                b["chargeKwh"] = b["dischargeKwh"] = 0;
            }
            JsonDocument response;
            assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
            f.mqtt.processBatteryMigration(testNow);
            assert(!f.mqtt._energyBacklog.batteryMigrationPending());
            f.mqtt._mqtt.messages.clear();
            if (reconnect)
            {
                f.mqtt._mqtt.online = false;
                assert(!f.mqtt._mqtt.connected());
                // The real loop postpones publishing while disconnected; pending configs must stay
                // unsent.
                assert(!f.mqtt.hasHaDiscoveryTopic(
                    buildDiscoveryTopic(getHaDeviceId(), "sensor", DESCR_Battery_Charge_Energy)));
                assert(!f.mqtt.hasHaDiscoveryTopic(buildDiscoveryTopic(
                    getHaDeviceId(), "sensor", DESCR_Battery_Discharge_Energy)));
                assert(countConfigs() == 0);
                f.mqtt._mqtt.online = true;
            }
            f.mqtt.publishState();
            f.mqtt.publishHaDiscovery(false);
            assert(countConfigs() == 2);
            f.mqtt._mqtt.messages.clear();
            f.mqtt.publishHaDiscovery(false);
            assert(countConfigs() == 0);
            assert(f.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 200);
            assert(response["state"] == "already_applied");
            f.mqtt.processBatteryMigration(testNow);
            f.mqtt.publishHaDiscovery(false);
            assert(countConfigs() == 0);
        }
    // An uncertain/failed write must keep the old discovery reservations until a verified retry.
    reset(false);
    Fixture failed(PI30_PIP_GK);
    failed.mqtt._energyBacklog.begin();
    failed.mqtt.initializeBatteryApi("test-board", "test-boot");
    failed.mqtt._energyBacklog.updateBatteryEnergy(failed.state, BatterySampleTiming{}, testNow);
    failed.mqtt.publishHaDiscovery(true);
    const auto topics = failed.mqtt._haDiscoveryTopics;
    storage.nvsWriteLimit = 7;
    auto b = request();
    JsonDocument response;
    assert(failed.mqtt.queueBatteryMigration(b.as<JsonObjectConst>(), response) == 202);
    failed.mqtt.processBatteryMigration(testNow);
    assert(failed.mqtt._energyBacklog.batteryMigrationPending());
    assert(failed.mqtt._haDiscoveryTopics == topics);
    std::puts("PASS: restore/zero-start discovery sends both unchanged energy configs once, "
              "including reconnect/idempotency and failed writes.");
}
} // namespace battery_cases
