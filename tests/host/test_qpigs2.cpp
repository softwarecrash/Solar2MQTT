#include <ArduinoJson.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <utility>
#include <vector>
#include "descriptors.h"

// UART/String substitutes only: production helpers, types and the complete
// production QPIGS2 parser are compiled below.
class String
{
  public:
    String() = default;
    String(const char *value) : text(value) {}
    size_t length() const
    {
        return text.length();
    }
    bool operator==(const char *value) const
    {
        return text == value;
    }
    void toCharArray(char *out, size_t size) const
    {
        if (!size)
            return;
        const size_t count = text.copy(out, size - 1);
        out[count] = '\0';
    }
    std::string text;
};
#include "pi_qpigs2_helpers_generated.h"
JsonDocument state;
JsonObject liveData;
class PI_Serial
{
  public:
    protocol_type_t protocol = PI30_PIP_GK;
    const char *delimiter = " ";
    struct
    {
        struct
        {
            String qpigs2;
        } raw;
    } get;
    String response;
    unsigned requests = 0, refinements = 0;
    String requestData(const char *command)
    {
        assert(std::strcmp(command, "QPIGS2") == 0);
        ++requests;
        return response;
    }
    void refineProtocol()
    {
        ++refinements;
    }
    bool PIXX_QPIGS2();
};
#include "PI_Serial/QPIGS2.h"

static const char *legacy =
    "0005 079.1 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477";
static const char *shortFrame = "01.9 388.9 00745";
static const char *pi41Frame = "230.0 50.0 230.0 50.0 1000 0900 020 0005 081.6 47.20 00100100";

static void reset()
{
    state.clear();
    liveData = state.to<JsonObject>();
    liveData[DESCR_PV_Input_Power] = 223;
    liveData[DESCR_PV_Input_Voltage] = 88.1;
    liveData[DESCR_Battery_Voltage] = 47.4;
    liveData[DESCR_AC_In_Voltage] = 235.5;
}
static void expect(const char *key, double want)
{
    if (liveData[key].isNull() || !std::isfinite(liveData[key].as<double>()) ||
        std::fabs(liveData[key].as<double>() - want) > 0.00001)
    {
        std::fprintf(stderr, "%s: null=%d value=%.6f expected=%.6f\n", key, liveData[key].isNull(),
                     liveData[key].as<double>(), want);
        std::abort();
    }
}
static void unchangedFirstTracker()
{
    expect(DESCR_PV_Input_Power, 223);
    expect(DESCR_PV_Input_Voltage, 88.1);
    expect(DESCR_Battery_Voltage, 47.4);
    expect(DESCR_AC_In_Voltage, 235.5);
    assert(liveData[DESCR_PV1_Input_Power].isNull());
    assert(liveData[DESCR_PV1_Input_Voltage].isNull());
}
static void noPv2()
{
    for (const char *key :
         {DESCR_PV2_Input_Current, DESCR_PV2_Input_Voltage, DESCR_PV2_Charging_Power,
          DESCR_PV2_Input_Power, DESCR_Battery_SCC2_Volt, DESCR_PV_Total_Input_Power})
        assert(liveData[key].isNull());
    unchangedFirstTracker();
}
static void snapshot(const char *name)
{
    std::string json;
    serializeJson(state, json);
    std::printf("%s %s\n", name, json.c_str());
}
static void existingFormats()
{
    reset();
    PI_Serial inverter;
    inverter.protocol = PI30_MAX;
    inverter.response = shortFrame;
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 1.9);
    expect(DESCR_PV2_Input_Voltage, 388.9);
    expect(DESCR_PV2_Charging_Power, 745);
    // Preserve the original MAX behavior: round(388.9 * 1.9), NOT reported watts.
    expect(DESCR_PV2_Input_Power, 739);
    unchangedFirstTracker();
    assert(liveData[DESCR_PV_Total_Input_Power].isNull());
    snapshot("MAX");
    inverter.response = "01.900 388.900 745.0";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 1.9);
    expect(DESCR_PV2_Input_Voltage, 388.9);
    expect(DESCR_PV2_Input_Power, 739);
    expect(DESCR_PV2_Charging_Power, 745);
    unchangedFirstTracker();
    snapshot("MAX-extra-precision");
    // Live .24 night reply captured during this verification.
    inverter.response = "00.0 000.0 00000";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_PV2_Input_Voltage, 0);
    expect(DESCR_PV2_Charging_Power, 0);
    expect(DESCR_PV2_Input_Power, 0);
    unchangedFirstTracker();
    snapshot("MAX-night");
    reset();
    inverter.protocol = PI41;
    inverter.response = pi41Frame;
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_AC_In_Voltage_L2, 230);
    expect(DESCR_AC_In_Frequency_L2, 50);
    expect(DESCR_AC_Out_Voltage_L2, 230);
    expect(DESCR_AC_Out_Frequency_L2, 50);
    expect(DESCR_AC_Out_VA_L2, 1000);
    expect(DESCR_AC_Out_Watt_L2, 900);
    expect(DESCR_AC_Out_Percent_L2, 20);
    expect(DESCR_PV2_Input_Current, 5);
    expect(DESCR_PV2_Input_Voltage, 81.6);
    expect(DESCR_Battery_Voltage_L2, 47.2);
    expect(DESCR_Device_Status_L2, 100100);
    expect(DESCR_PV2_Input_Power, 408);
    unchangedFirstTracker();
    snapshot("PI41");
    inverter.response =
        "230.00 50.000 230.0 50.00 1000.0 0900.00 020.0 0005.000 081.600 47.200 00100100";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 5);
    expect(DESCR_PV2_Input_Voltage, 81.6);
    expect(DESCR_PV2_Input_Power, 408);
    expect(DESCR_Battery_Voltage_L2, 47.2);
    unchangedFirstTracker();
    snapshot("PI41-extra-precision");
}

static std::string replaceField(const char *frame, size_t index, const std::string &replacement)
{
    std::istringstream input(frame);
    std::string word, result;
    for (size_t i = 0; input >> word; ++i)
    {
        if (!result.empty())
            result += ' ';
        result += i == index ? replacement : word;
    }
    return result;
}
static void expectLegacyValues()
{
    expect(DESCR_PV2_Input_Voltage, 79.1);
    expect(DESCR_Battery_SCC2_Volt, 47.2);
    expect(DESCR_PV2_Charging_Power, 254);
    expect(DESCR_PV2_Input_Power, 254);
    expect(DESCR_PV2_Input_Current, 3.21);
    expect(DESCR_PV_Total_Input_Power, 477);
    unchangedFirstTracker();
}
static void recordedSnapshots()
{
    reset();
    PI_Serial inverter;
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    expectLegacyValues();
    snapshot("GK-captured-day-1");
    inverter.response = "0005 091.8 49.30 00256 10000000 0000 0000 0000 000.0 00.00 00000 01018";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Voltage, 91.8);
    expect(DESCR_PV2_Input_Current, 2.79);
    expect(DESCR_PV_Total_Input_Power, 1018);
    unchangedFirstTracker();
    snapshot("GK-captured-day-2");
    inverter.response = "0000 000.0 47.20 00000 00000000 0000 0000 0000 000.0 00.00 00000 00000";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_PV2_Input_Power, 0);
    unchangedFirstTracker();
    snapshot("GK-night");
}
static void numberFormats()
{
    const std::vector<std::pair<size_t, std::vector<const char *>>> equivalents = {
        {0, {"5", "5.0", "0005.000000"}},        {1, {"79.1", "079.10", "79.100000"}},
        {2, {"47.2", "47.20", "47.200000"}},     {3, {"254", "254.0", "00254.000000"}},
        {5, {"0", "0.0", "0000.000000"}},        {6, {"0", "0.00", "0000000000000000000000"}},
        {7, {"0", "0.0", "0000.000000"}},        {8, {"0", "0.00", "000.000000"}},
        {9, {"0", "0.0", "00.000000"}},          {10, {"0", "0.000", "00000.000000"}},
        {11, {"477", "0477.0", "000477.000000"}}};
    unsigned variants = 0;
    for (const auto &field : equivalents)
        for (const char *value : field.second)
        {
            reset();
            PI_Serial inverter;
            const auto reply = replaceField(legacy, field.first, value);
            inverter.response = reply.c_str();
            assert(inverter.PIXX_QPIGS2());
            expectLegacyValues();
            assert(inverter.get.raw.qpigs2.text == reply);
            assert(inverter.refinements == 1);
            ++variants;
        }
    reset();
    PI_Serial inverter;
    inverter.response = "5.0 79.100 47.2 254.00 10000000 0.0 0.000 0.0 0 0.0 0.0 477.000";
    assert(inverter.PIXX_QPIGS2());
    expectLegacyValues();
    inverter.response = "0 0 0 0 00000000 0 0 0 0 0 0 0";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Power, 0);
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_Battery_SCC2_Volt, 0);
    expect(DESCR_PV_Total_Input_Power, 0);
    unchangedFirstTracker();
    inverter.response = "5.0 0.000 47.2 254.00 10000000 0 0 0 0 0 0 477";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_PV2_Input_Power, 254);
    unchangedFirstTracker();
    const auto integralVoltage = replaceField(legacy, 1, "079");
    inverter.response = integralVoltage.c_str();
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Voltage, 79);
    expect(DESCR_PV2_Input_Current, 3.22);
    // Preserve the production two-decimal conversion; this change only accepts
    // alternate input representations, not additional stored precision.
    inverter.response = "5.125 79.125 47.236 254.125 10000000 0 0 0 0 0 0 477.125";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Voltage, 79.13);
    expect(DESCR_Battery_SCC2_Volt, 47.24);
    expect(DESCR_PV2_Input_Power, 254.13);
    expect(DESCR_PV_Total_Input_Power, 477.13);
    expect(DESCR_PV2_Input_Current, 3.21);
    const auto upperValue = replaceField(legacy, 11, "00099999.000");
    inverter.response = upperValue.c_str();
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV_Total_Input_Power, 99999);
    const size_t padding = 127 - std::strlen(legacy);
    const auto atLimit = replaceField(legacy, 1, std::string("079.1") + std::string(padding, '0'));
    assert(atLimit.size() == 127);
    inverter.response = atLimit.c_str();
    assert(inverter.PIXX_QPIGS2());
    expectLegacyValues();
    const auto overLimit =
        replaceField(legacy, 1, std::string("079.1") + std::string(padding + 1, '0'));
    assert(overLimit.size() == 128);
    inverter.response = overLimit.c_str();
    assert(inverter.PIXX_QPIGS2());
    noPv2();
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    expectLegacyValues();
    std::printf("PASS: %u equivalent field representations, mixed precision, integer zero, "
                "rounding and 127/128-byte limits.\n",
                variants);
}
static void invalidNumbers()
{
    const std::vector<const char *> invalid = {
        "",   "NaN", "inf",  "-1", "-0.0", "+1",  "1e2",    "1e400",      "1,0",
        ".5", "5.",  "1..0", "1x", "0x10", "1\t", "100000", "000100000.0"};
    unsigned cases = 0;
    reset();
    PI_Serial inverter;
    for (size_t field = 0; field < 12; ++field)
    {
        if (field == 4)
            continue;
        for (const char *value : invalid)
        {
            inverter.response = legacy;
            assert(inverter.PIXX_QPIGS2());
            const auto before = inverter.refinements;
            const auto reply = replaceField(legacy, field, value);
            inverter.response = reply.c_str();
            assert(inverter.PIXX_QPIGS2());
            noPv2();
            assert(inverter.refinements == before);
            assert(inverter.get.raw.qpigs2.text == reply);
            ++cases;
        }
    }
    for (const char *bits : {"1000000", "100000000", "10000002", "abcdefgh", "1000.000"})
    {
        inverter.response = legacy;
        assert(inverter.PIXX_QPIGS2());
        const auto reply = replaceField(legacy, 4, bits);
        inverter.response = reply.c_str();
        assert(inverter.PIXX_QPIGS2());
        noPv2();
    }
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    expectLegacyValues();
    std::printf("PASS: %u invalid numeric/empty-field cases, strict eight-bit status, stale "
                "clearing and recovery.\n",
                cases);
}
static void errorRepliesForEachLayout()
{
    struct Layout
    {
        protocol_type_t protocol;
        const char *reply;
        double power;
    };
    const std::vector<Layout> layouts = {
        {PI30_MAX, shortFrame, 739}, {PI30_PIP_GK, legacy, 254}, {PI41, pi41Frame, 408}};
    const std::vector<std::string> failures = {"NAK", "NOA",     "ERCRC",
                                               "",    "garbage", std::string(128, '0')};
    for (const auto &layout : layouts)
        for (const auto &failure : failures)
        {
            reset();
            PI_Serial inverter;
            inverter.protocol = layout.protocol;
            inverter.response = layout.reply;
            assert(inverter.PIXX_QPIGS2());
            expect(DESCR_PV2_Input_Power, layout.power);
            const auto before = inverter.refinements;
            inverter.response = failure.c_str();
            assert(inverter.PIXX_QPIGS2() == (failure != "ERCRC"));
            if (failure == "ERCRC")
            {
                // A corrupted reply leaves the previous values for cycle rollback.
                expect(DESCR_PV2_Input_Power, layout.power);
            }
            else
            {
                noPv2();
                for (const char *key : qpigs2ListL2)
                    assert(liveData[key].isNull());
            }
            assert(inverter.refinements == before);
            inverter.response = layout.reply;
            assert(inverter.PIXX_QPIGS2());
            expect(DESCR_PV2_Input_Power, layout.power);
            unchangedFirstTracker();
        }
    std::puts(
        "PASS: 24er/25er/PI41 NAK, NOA, CRC, empty, malformed, oversized replies and recovery.");
}

int main(int argc, char **argv)
{
    existingFormats();
    if (argc == 2 && std::strcmp(argv[1], "--compat") == 0)
        return 0;
    if (argc == 2 && std::strcmp(argv[1], "--recorded") == 0)
    {
        recordedSnapshots();
        return 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--formats") == 0)
    {
        numberFormats();
        invalidNumbers();
        errorRepliesForEachLayout();
        return 0;
    }
    reset();
    PI_Serial inverter;
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    assert(inverter.get.raw.qpigs2.text == legacy);
    expect(DESCR_PV2_Input_Voltage, 79.1);
    expect(DESCR_Battery_SCC2_Volt, 47.2);
    expect(DESCR_PV2_Charging_Power, 254);
    expect(DESCR_PV2_Input_Power, 254);
    expect(DESCR_PV2_Input_Current, 3.21);
    expect(DESCR_PV_Total_Input_Power, 477);
    assert(inverter.refinements == 1);
    unchangedFirstTracker();
    // Second captured .25 response, from before the first OTA.
    inverter.response = "0005 091.8 49.30 00256 10000000 0000 0000 0000 000.0 00.00 00000 01018";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Power, 256);
    expect(DESCR_PV2_Input_Current, 2.79);
    expect(DESCR_PV_Total_Input_Power, 1018);
    inverter.response = "0000 000.0 47.20 00000 00000000 0000 0000 0000 000.0 00.00 00000 00000";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_PV2_Input_Power, 0);
    expect(DESCR_PV_Total_Input_Power, 0);
    inverter.response = "0005 000.0 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477";
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Current, 0);
    expect(DESCR_PV2_Input_Power, 254);
    // A later short reply must not retain fields owned by the twelve-field layout.
    inverter.protocol = PI30_MAX;
    inverter.response = shortFrame;
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Power, 739);
    assert(liveData[DESCR_Battery_SCC2_Volt].isNull());
    assert(liveData[DESCR_PV_Total_Input_Power].isNull());
    inverter.protocol = PI30_PIP_GK;
    const std::vector<std::string> rejected = {
        "NAK",
        "NOA",
        "",
        "garbage",
        "0005 079.1 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000",
        std::string(legacy) + " 0000",
        std::string(legacy) + std::string(200, ' '),
        "0005 NaN 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477",
        "0005 inf 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477",
        "0005 -79.1 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477",
        "0005 079.1 47.20 00254 10000002 0000 0000 0000 000.0 00.00 00000 00477",
        "0005 079.1 47.20 00254 1000000 0000 0000 0000 000.0 00.00 00000 00477",
        "0005 079.1 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 0047x",
        "235.8 50.0 235.8 50.0 0023 0008 000 385 47.40 010 052 0040 0004 090.4 47.40 00000 "
        "00010110 00 00 00218 010",
        pi41Frame};
    for (const std::string &reply : rejected)
    {
        inverter.response = legacy;
        assert(inverter.PIXX_QPIGS2());
        inverter.response = reply.c_str();
        assert(inverter.PIXX_QPIGS2());
        assert(inverter.get.raw.qpigs2.text == reply);
        noPv2();
    }
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    inverter.response = "ERCRC";
    assert(!inverter.PIXX_QPIGS2());
    expectLegacyValues();
    inverter.response = legacy;
    assert(inverter.PIXX_QPIGS2());
    expect(DESCR_PV2_Input_Power, 254);
    for (protocol_type_t protocol : {PI30, PI30_MAX, PI30_REVO, PI30_PIP_GK, PI30_UNKNOWN, PI41})
    {
        inverter.protocol = protocol;
        inverter.response = legacy;
        assert(inverter.PIXX_QPIGS2());
        expect(DESCR_PV2_Input_Power, 254);
    }
    const unsigned requests = inverter.requests;
    inverter.protocol = PI16;
    assert(inverter.PIXX_QPIGS2());
    assert(inverter.get.raw.qpigs2 == DESCR_req_NOA);
    inverter.protocol = PI18;
    assert(inverter.PIXX_QPIGS2());
    inverter.protocol = NoD;
    assert(!inverter.PIXX_QPIGS2());
    assert(inverter.requests == requests);
    numberFormats();
    invalidNumbers();
    errorRepliesForEachLayout();
    std::puts(
        "PASS: captured .24/.25 PV2 replies, unchanged MAX/PI41 behavior and names, twelve-field "
        "validation, zero/NAK/NOA/CRC, stale fields, recovery and protocol dispatch.");
}
