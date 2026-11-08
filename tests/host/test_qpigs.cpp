#include <ArduinoJson.h>
#include <cassert>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "descriptors.h"

// Only String and UART are substituted. Compile the complete production
// QPIGS parser, production field helpers, types and ArduinoJson.
class String {
public:
    String() = default;
    String(const char *value) : text(value ? value : "") {}
    String(const std::string &value) : text(value) {}
    bool isEmpty() const { return text.empty(); }
    size_t length() const { return text.size(); }
    bool operator==(const char *value) const { return text == value; }
    bool operator!=(const char *value) const { return text != value; }
    bool startsWith(const char *prefix) const { return text.compare(0, std::strlen(prefix), prefix) == 0; }
    String substring(size_t first) const { return first < text.size() ? String(text.substr(first)) : String(); }
    long toInt() const { return std::strtol(text.c_str(), nullptr, 10); }
    char charAt(size_t index) const { return index < text.size() ? text[index] : '\0'; }
    bool equalsIgnoreCase(const char *other) const {
        if (text.size() != std::strlen(other)) return false;
        for (size_t i = 0; i < text.size(); ++i)
            if (std::tolower(static_cast<unsigned char>(text[i])) != std::tolower(static_cast<unsigned char>(other[i]))) return false;
        return true;
    }
    void toCharArray(char *out, size_t size) const {
        if (size) out[text.copy(out, size - 1)] = '\0';
    }
    std::string text;
};
namespace ArduinoJson {
template <> struct Converter<String> {
    static void toJson(const String &value, JsonVariant out) { out.set(value.text.c_str()); }
};
}
#include "pi_qpigs_helpers_generated.h"
inline bool isDigit(unsigned char value) { return std::isdigit(value) != 0; }
void writeLog(const char *, ...) {}
JsonDocument state;
JsonObject liveData;
class PI_Serial {
public:
    protocol_type_t protocol = PI30_PIP_GK;
    const char *delimiter = " ";
    protocol_type_t forcedProtocol = PI30_PIP_GK;
    struct { struct { String qall, qpigs, qpigs2, qpiri, qpi, qflag, qmod, qpiws; } raw; } get;
    String qallReply = "NAK", qpigsReply;
    std::vector<std::string> commands;
    String requestData(const char *command) {
        commands.emplace_back(command);
        if (std::strcmp(command, "QALL") == 0) return qallReply;
        assert(std::strcmp(command, "QPIGS") == 0);
        return qpigsReply;
    }
    void refineProtocol();
    bool isValidResponse(const String &response) const;
    char *getModeDesc(char mode);
    bool PIXX_QPIGS();
};
#include "PI_Serial/QPIGS.h"
#include "pi_qpigs_mode_generated.h"

// MAX and PIP-GK status replies from the existing QPIGS regression cases.
static const char *gkRating = "240.0 20.8 240.0 50.0 21.7 5000 4000 48.0 44.0 42.0 56.4 54.0 0 10 080 0 2 3 6 20 0 0 48.0 0 1";
static const char *maxRating = "230.0 47.8 230.0 50.0 47.8 11000 11000 48.0 44.0 44.0 56.4 54.0 0 002 080 0 2 3 6 01 0 0 48.0 0 1 480 0 150";
static const char *gkNight = "236.8 49.9 236.8 49.9 0023 0008 000 367 45.20 000 030 0038 0000 000.0 00.00 00000 00010000 00 00 00000 010";
static const char *gkDay = "235.8 50.0 235.8 50.0 0023 0008 000 385 47.40 010 052 0040 0004 090.4 47.40 00000 00010110 00 00 00218 010";
static const char *maxNight = "233.6 49.9 233.6 49.9 00677 00573 006 404 45.30 000 000 0038 00.0 000.0 00.00 00000 00010000 00 00 00000 010 0 00 0000";
static const char *maxDay = "232.4 50.2 232.4 50.2 02440 02415 022 414 47.50 000 033 0041 01.8 391.9 00.00 00000 00010110 00 00 00721 010 0 00 0000";
// REVO protocol V03, sections 1.2/1.4: constructed from its documented example
// field values and widths, NOT a capture from physical REVO hardware.
static const char *revoStatus = "234.0 50.0 230.0 49.9 0000 0736 013 401 50.00 010 081 0026 0622 311.0 53.00 16850 00000110 00 00 00622 010";
static const char *revoAll = "234 50.0 230 49.9 0736 013 51.2 098 101 138 289 15.3 1385 016850 000368 L 00 00";

static std::string fields(const std::string &frame, size_t count, int index = -1, const char *replacement = "") {
    std::istringstream input(frame);
    std::string value, result;
    for (size_t i = 0; i < count; ++i) {
        assert(input >> value);
        if (!result.empty()) result += ' ';
        result += static_cast<int>(i) == index ? replacement : value;
    }
    return result;
}
static void reset() {
    state.clear(); liveData = state.to<JsonObject>();
    liveData[DESCR_Inverter_Operation_Mode] = "Line";
}
static std::string json() { std::string out; serializeJson(state, out); return out; }
static void expect(const char *key, double value) {
    if (liveData[key].isNull() || std::fabs(liveData[key].as<double>() - value) > 0.000001) {
        std::fprintf(stderr, "%s: null=%d value=%.6f expected=%.6f\n", key, liveData[key].isNull(), liveData[key].as<double>(), value);
        std::abort();
    }
}
static void absent(const char *key) { assert(liveData[key].isNull()); }
static PI_Serial standard(protocol_type_t protocol = PI30_PIP_GK) {
    PI_Serial inverter; inverter.protocol = protocol;
    inverter.forcedProtocol = protocol;
    inverter.get.raw.qpi = protocol == PI41 ? "PI41" : "PI30";
    inverter.get.raw.qpiri = gkRating; inverter.qpigsReply = gkDay;
    return inverter;
}
static void expectStandard(double current, double discharge, double power = 218, double voltage = 90.4) {
    expect(DESCR_PV1_Input_Current, current); expect(DESCR_Battery_Discharge_Current, discharge);
    expect(DESCR_PV_Input_Power, power); expect(DESCR_PV_Input_Voltage, voltage);
    absent(DESCR_PV_Generation_Day);
    // Keep the established, mixed PV/PV1 naming. No new topic aliases.
    assert(!liveData["PV_Input_Power"].isNull());
    absent(DESCR_PV1_Input_Power); absent(DESCR_PV1_Input_Voltage);
    assert(std::strcmp(liveData[DESCR_Inverter_Operation_Mode], "Line") == 0);
}
static void snapshot(const char *label, bool enabled) {
    if (enabled) std::printf("%s %s\n", label, json().c_str());
}

static void compatibility(bool snapshots) {
    reset(); auto inverter = standard(PI30_MAX);
    inverter.get.raw.qpiri = maxRating; inverter.qpigsReply = maxNight;
    assert(inverter.PIXX_QPIGS()); expectStandard(0, 0, 0, 0); snapshot("MAX-24-night", snapshots);
    reset(); inverter.qpigsReply = maxDay;
    assert(inverter.PIXX_QPIGS()); expectStandard(1.8, 0, 721, 391.9); snapshot("MAX-24-day", snapshots);
    for (auto protocol : {PI16, PI41}) {
        reset(); inverter = standard(protocol);
        inverter.qpigsReply = fields(gkDay, 21, 12, "04.0");
        assert(inverter.PIXX_QPIGS()); expectStandard(4, 0);
        snapshot(protocol == PI16 ? "PI16" : "PI41", snapshots);
    }
    reset(); inverter = standard(PI30); inverter.qpigsReply = fields(gkDay, 17, 12, "04.0");
    assert(inverter.PIXX_QPIGS()); expectStandard(4, 0, 362); snapshot("PI30-17-fields", snapshots);
    for (auto protocol : {PI30_REVO, PI30, PI30_UNKNOWN}) {
        reset(); inverter = standard(protocol); inverter.get.raw.qpiri = fields(gkRating, 21);
        inverter.qpigsReply = revoStatus; inverter.qallReply = revoAll;
        assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
        expect(DESCR_Battery_Discharge_Current, 138); expect(DESCR_PV1_Input_Current, 15.3);
        expect(DESCR_PV_Charging_Power, 1385);
        snapshot(protocol == PI30_REVO ? "REVO-explicit-QALL" : protocol == PI30 ? "REVO-inferred-PI30" : "REVO-inferred-unknown", snapshots);
    }
    for (const char *qall : {"NAK", "NOA", ""}) {
        reset(); inverter = standard(PI30_REVO); inverter.qallReply = qall; inverter.qpigsReply = revoStatus;
        assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
        expect(DESCR_PV_Input_Power, 622); expect(DESCR_PV1_Input_Current, 2);
        absent(DESCR_Battery_Discharge_Current); snapshot("REVO-without-QALL", snapshots);
    }
    reset(); inverter = standard(PI30_REVO); inverter.get.raw.qpiri = fields(gkRating, 21);
    inverter.qallReply = revoAll; inverter.qpigsReply = fields(revoStatus, 21, 12, "0622.0");
    assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
    expect(DESCR_Battery_Discharge_Current, 138); expect(DESCR_PV1_Input_Current, 15.3);
    snapshot("REVO-explicit-decimal-QALL", snapshots);
    for (auto protocol : {PI30, PI30_UNKNOWN}) {
        for (const char *qall : {"NAK", "NOA", ""}) {
            reset(); inverter = standard(protocol); inverter.get.raw.qpiri = fields(gkRating, 21);
            inverter.qpigsReply = revoStatus; inverter.qallReply = qall;
            assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
            expect(DESCR_PV_Input_Power, 622); absent(DESCR_Battery_Discharge_Current);
            snapshot(protocol == PI30 ? "REVO-fallback-PI30-no-QALL" : "REVO-fallback-unknown-no-QALL", snapshots);
        }
    }
}
static void regression() {
    reset(); auto inverter = standard();
    // Change only documented discharge current to synthetic 7 A.
    inverter.qpigsReply = fields(gkNight, 21, 15, "00007");
    assert(inverter.PIXX_QPIGS()); expectStandard(0, 7, 0, 0);
    expect(DESCR_Battery_Load, -7);
    inverter.qpigsReply = fields(inverter.qpigsReply.text, 21, 12, "00.0");
    assert(inverter.PIXX_QPIGS()); expectStandard(0, 7, 0, 0);
    std::puts("QPIGS: captured GK integer/decimal zero, 7 A discharge without invented energy passed");
}
static void layoutPrecedence() {
    // Even a conflicting cached rating/QALL signature must not override an
    // explicitly recognized standard protocol. Only unclassified PI30 infers.
    for (auto protocol : {PI30_PIP_GK, PI30_MAX, PI41, PI16}) {
        for (const char *current : {"0004", "04.0"}) {
            for (const char *qall : {"NAK", "NOA", "", revoAll}) {
                reset(); auto inverter = standard(protocol); inverter.qallReply = qall;
                inverter.get.raw.qpiri = fields(gkRating, 21);
                inverter.qpigsReply = fields(fields(gkDay, 21, 15, "00007"), 21, 12, current);
                assert(inverter.PIXX_QPIGS());
                expectStandard(std::strchr(current, '.') ? 4 : 2.41, 7);
                expect(DESCR_Battery_Load, 3);
                assert(inverter.get.raw.qpigs.text == inverter.qpigsReply.text);
            }
        }
    }
    for (auto protocol : {PI30, PI30_UNKNOWN}) {
        for (const char *qall : {"NAK", "NOA", ""}) {
            for (size_t ratingCount : {size_t(25)}) {
                reset(); auto inverter = standard(protocol); inverter.qallReply = qall;
                inverter.get.raw.qpiri = fields(gkRating, ratingCount);
                inverter.qpigsReply = fields(gkDay, 21, 15, "00007");
                assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 7);
            }
        }
        for (size_t ratingCount : {size_t(0), size_t(22), size_t(25)}) {
            reset(); auto inverter = standard(protocol); inverter.qallReply = revoAll;
            inverter.get.raw.qpiri = fields(gkRating, ratingCount);
            assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 0);
        }
        reset(); auto inverter = standard(protocol); inverter.get.raw.qpiri = fields(gkRating, 21);
        inverter.qallReply = fields(revoAll, 17);
        assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 0);
    }
    std::puts("QPIGS: known-protocol precedence, integer/decimal positive current and complete REVO signature passed");
}
static void revoFallback() {
    for (auto protocol : {PI30, PI30_UNKNOWN}) {
        for (const char *qall : {"NAK", "NOA", ""}) {
            for (size_t ratingCount : {size_t(0), size_t(21), size_t(24)}) {
                reset(); auto inverter = standard(protocol);
                inverter.get.raw.qpiri = fields(gkRating, ratingCount);
                inverter.qallReply = qall; inverter.qpigsReply = revoStatus;
                assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
                expect(DESCR_PV_Input_Power, 622); absent(DESCR_Battery_Discharge_Current);
            }
        }
        for (int mutation : {0, 1, 2}) {
            reset(); auto inverter = standard(protocol); inverter.get.raw.qpiri = fields(gkRating, 21);
            inverter.qpigsReply = mutation == 0 ? fields(revoStatus, 21, 12, "0622.0") :
                mutation == 1 ? fields(revoStatus, 21, 13, "311") : fields(revoStatus, 21, 16, "00000112");
            assert(inverter.PIXX_QPIGS()); absent(DESCR_PV_Generation_Day);
        }
    }
    std::puts("QPIGS: legacy no-QALL REVO fallback retained; missing/short rating and original signature guards passed");
}
static void firstDetectionCycle() {
    // Replay QPI and static ratings before the first QPIGS.
    // QMOD/QPIWS are still empty during the first QPIGS. Execute the complete
    // production refineProtocol rather than the previous no-op substitute.
    for (auto initial : {PI30, PI30_UNKNOWN}) {
        for (const char *frame : {gkNight, gkDay}) {
            for (const char *qall : {"NAK", "NOA"}) {
                reset(); auto inverter = standard(initial); inverter.forcedProtocol = NoD;
                inverter.qallReply = qall; inverter.qpigsReply = fields(frame, 21, 15, "00007");
                const bool night = frame == gkNight;
                assert(inverter.get.raw.qpiws.isEmpty()); assert(inverter.get.raw.qmod.isEmpty());
                assert(inverter.PIXX_QPIGS());
                expectStandard(night ? 0 : 2.41, 7, night ? 0 : 218, night ? 0 : 90.4);
                assert(inverter.protocol == PI30);
                inverter.get.raw.qmod = "L";
                inverter.get.raw.qpiws = "00000000001000000000000000000000000000";
                inverter.refineProtocol(); assert(inverter.protocol == PI30_PIP_GK);
                assert(inverter.PIXX_QPIGS());
                expectStandard(night ? 0 : 2.41, 7, night ? 0 : 218, night ? 0 : 90.4);
            }
        }
        for (const char *frame : {maxNight, maxDay}) {
            reset(); auto inverter = standard(initial); inverter.forcedProtocol = NoD;
            inverter.get.raw.qpiri = maxRating; inverter.qpigsReply = frame;
            assert(inverter.PIXX_QPIGS()); assert(inverter.protocol == PI30_MAX);
            expectStandard(frame == maxNight ? 0 : 1.8, 0, frame == maxNight ? 0 : 721, frame == maxNight ? 0 : 391.9);
        }
        for (const char *qall : {"NAK", "NOA", ""}) {
            reset(); auto inverter = standard(initial); inverter.forcedProtocol = NoD;
            inverter.get.raw.qpiri = fields(gkRating, 21); inverter.qpigsReply = revoStatus; inverter.qallReply = qall;
            assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850); absent(DESCR_Battery_Discharge_Current);
            assert(inverter.protocol == PI30);
            inverter.get.raw.qpiws = "00000000000000000000000000000000000000";
            inverter.refineProtocol(); assert(inverter.protocol == PI30);
            assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
            inverter.qallReply = revoAll;
            assert(inverter.PIXX_QPIGS()); assert(inverter.protocol == PI30_REVO);
            inverter.qallReply = "NAK";
            assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850); absent(DESCR_Battery_Discharge_Current);
        }
        for (bool badQall : {false, true}) {
            reset(); auto inverter = standard(initial); inverter.forcedProtocol = NoD;
            const auto before = json();
            if (badQall) inverter.qallReply = "ERCRC"; else inverter.qpigsReply = "ERCRC";
            assert(!inverter.PIXX_QPIGS()); assert(before == json()); assert(inverter.protocol == initial);
            assert(inverter.commands.size() == (badQall ? 1 : 2));
            inverter.qallReply = "NAK"; inverter.qpigsReply = fields(gkDay, 21, 15, "00007");
            assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 7);
            assert(inverter.protocol == PI30);
        }
    }
    std::puts("QPIGS: first cycle for PIP-GK, MAX and REVO, later QPIWS and QALL recovery passed");
}
static void failuresAndRecovery() {
    for (const char *failure : {"NAK", "NOA", "ERCRC"}) {
        for (bool failQall : {false, true}) {
            reset(); auto inverter = standard();
            assert(inverter.PIXX_QPIGS()); const auto before = json(); inverter.commands.clear();
            if (failQall) inverter.qallReply = failure; else inverter.qpigsReply = failure;
            const bool success = inverter.PIXX_QPIGS();
            assert(success == (std::strcmp(failure, "ERCRC") != 0));
            if (!failQall || !success) assert(before == json());
            assert(inverter.commands.size() == (failQall && !success ? 1 : 2));
            inverter.qallReply = "NAK"; inverter.qpigsReply = gkDay;
            assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 0);
        }
    }
    reset(); auto inverter = standard(PI30_REVO); inverter.qpigsReply = revoStatus;
    assert(inverter.PIXX_QPIGS()); expect(DESCR_PV_Generation_Day, 16850);
    inverter.protocol = PI30_PIP_GK; inverter.qpigsReply = fields(gkDay, 21, 15, "00007");
    assert(inverter.PIXX_QPIGS()); expectStandard(2.41, 7);
    for (const char *shortReply : {"", "235.8", "235.8 50.0"}) {
        reset(); inverter = standard(PI30); inverter.get.raw.qpiri = fields(gkRating, 21);
        inverter.qallReply = revoAll; inverter.qpigsReply = shortReply;
        assert(inverter.PIXX_QPIGS()); absent(DESCR_PV_Generation_Day);
    }
    std::puts("QPIGS: NAK, silence, CRC failures, incomplete signatures, stale layout clearing and recovery passed");
}
int main(int argc, char **argv) {
    if (argc == 2 && std::strcmp(argv[1], "--compat") == 0) { compatibility(true); return 0; }
    if (argc == 2 && std::strcmp(argv[1], "--startup") == 0) { firstDetectionCycle(); return 0; }
    regression(); compatibility(false); layoutPrecedence(); revoFallback(); firstDetectionCycle(); failuresAndRecovery();
    std::puts("QPIGS layout regression tests passed");
}
