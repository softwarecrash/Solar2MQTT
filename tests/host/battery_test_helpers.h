#pragma once

namespace battery_cases
{
using namespace battery_test;
constexpr const char *id = "1234567890abcdef1234567890abcdef",
                     *otherId = "22222222222222222222222222222222";
inline void reset(bool fs = true)
{
    storage = Storage{};
    storage.partitionPresent = fs;
    testNow = 100;
}
// Independent little-endian fixtures for both on-disk record formats.
inline std::vector<uint8_t> record(uint32_t seq, uint64_t charge, uint64_t discharge,
                                   bool legacy = false)
{
    std::vector<uint8_t> b(legacy ? 32 : 48, 0);
    auto put = [&](size_t at, uint64_t v, size_t n)
    {
        for (size_t i = 0; i < n; i++)
            b[at + i] = static_cast<uint8_t>(v >> (8 * i));
    };
    put(0, 0x42415445, 4);
    put(4, seq, 4);
    put(8, charge, 8);
    put(16, discharge, 8);
    put(legacy ? 24 : 40, legacy ? 1 : 2, 2);
    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < b.size() - 4; i++)
    {
        hash ^= b[i];
        hash *= 16777619UL;
    }
    put(b.size() - 4, hash, 4);
    return b;
}
inline void file(const char *name, const std::vector<uint8_t> &bytes)
{
    storage.files[name] = std::make_shared<std::vector<uint8_t>>(bytes);
}
inline void counters(EnergyBacklog &e, double charge, double discharge)
{
    e._batteryChargeWh = charge;
    e._batteryDischargeWh = discharge;
    e._batteryDirty = true;
}
inline BatterySampleTiming reading(uint32_t at, uint32_t seq, uint32_t lease = 30000)
{
    BatterySampleTiming s;
    s.available = true;
    s.sequence = seq;
    s.sampledAtMs = s.acceptedAtMs = at;
    s.validForMs = lease;
    return s;
}
inline void power(SolarState &state, double watts)
{
    auto live = state.doc()["LiveData"].as<JsonObject>();
    live[DESCR_Battery_Voltage] = 50;
    live[DESCR_Battery_Charge_Current] = watts > 0 ? watts / 50 : 0;
    live[DESCR_Battery_Discharge_Current] = watts < 0 ? -watts / 50 : 0;
    state.commitInverterState();
}
inline void sampleAt(EnergyBacklog &e, SolarState &state, uint32_t at, uint32_t seq, double watts)
{
    testNow = at;
    power(state, watts);
    e.updateBatteryEnergy(state, reading(at, seq), at);
}

} // namespace battery_cases
