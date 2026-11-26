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
// QPIGS/QPIGS2 parsers, production field helpers, types and ArduinoJson.
class String
{
  public:
    String() = default;
    String(const char *value) : text(value ? value : "") {}
    String(const std::string &value) : text(value) {}
    bool isEmpty() const
    {
        return text.empty();
    }
    size_t length() const
    {
        return text.size();
    }
    bool operator==(const char *value) const
    {
        return text == value;
    }
    bool operator!=(const char *value) const
    {
        return text != value;
    }
    bool startsWith(const char *prefix) const
    {
        return text.compare(0, std::strlen(prefix), prefix) == 0;
    }
    String substring(size_t first) const
    {
        return first < text.size() ? String(text.substr(first)) : String();
    }
    long toInt() const
    {
        return std::strtol(text.c_str(), nullptr, 10);
    }
    char charAt(size_t index) const
    {
        return index < text.size() ? text[index] : '\0';
    }
    bool equalsIgnoreCase(const char *other) const
    {
        if (text.size() != std::strlen(other))
            return false;
        for (size_t i = 0; i < text.size(); ++i)
            if (std::tolower(static_cast<unsigned char>(text[i])) !=
                std::tolower(static_cast<unsigned char>(other[i])))
                return false;
        return true;
    }
    void toCharArray(char *out, size_t size) const
    {
        if (size)
            out[text.copy(out, size - 1)] = '\0';
    }
    std::string text;
};
namespace ArduinoJson
{
template <> struct Converter<String>
{
    static void toJson(const String &value, JsonVariant out)
    {
        out.set(value.text.c_str());
    }
};
} // namespace ArduinoJson
#include "pi_qpigs_helpers_generated.h"
inline bool isDigit(unsigned char value)
{
    return std::isdigit(value) != 0;
}
void writeLog(const char *, ...) {}
JsonDocument state;
JsonObject liveData;
class PI_Serial
{
  public:
    protocol_type_t protocol = PI30_PIP_GK;
    const char *delimiter = " ";
    protocol_type_t forcedProtocol = PI30_PIP_GK;
    struct
    {
        struct
        {
            String qall, qpigs, qpigs2, qpiri, qpi, qflag, qmod, qpiws;
        } raw;
    } get;
    String qallReply = "NAK", qpigsReply, qpigs2Reply = "NAK";
    std::vector<std::string> commands;
    String requestData(const char *command)
    {
        commands.emplace_back(command);
        if (std::strcmp(command, "QALL") == 0)
            return qallReply;
        if (std::strcmp(command, "QPIGS") == 0)
            return qpigsReply;
        assert(std::strcmp(command, "QPIGS2") == 0);
        return qpigs2Reply;
    }
    void refineProtocol();
    bool isValidResponse(const String &response) const;
    char *getModeDesc(char mode);
    bool PIXX_QPIGS();
    bool PIXX_QPIGS2();
};
#include "PI_Serial/QPIGS.h"
#include "PI_Serial/QPIGS2.h"
#include "pi_qpigs_mode_generated.h"

// Recorded replies already present in the repository's parser regressions.
static const char *gkRating = "240.0 20.8 240.0 50.0 21.7 5000 4000 48.0 44.0 42.0 56.4 54.0 0 10 "
                              "080 0 2 3 6 20 0 0 48.0 0 1";
static const char *maxRating = "230.0 47.8 230.0 50.0 47.8 11000 11000 48.0 44.0 44.0 56.4 54.0 0 "
                               "002 080 0 2 3 6 01 0 0 48.0 0 1 480 0 150";
static const char *gkDay = "235.8 50.0 235.8 50.0 0023 0008 000 385 47.40 010 052 0040 0004 090.4 "
                           "47.40 00000 00010110 00 00 00218 010";
static const char *maxDay = "232.4 50.2 232.4 50.2 02440 02415 022 414 47.50 000 033 0041 01.8 "
                            "391.9 00.00 00000 00010110 00 00 00721 010 0 00 0000";
static const char *legacy = "0005 079.1 47.20 00254 10000000 0000 0000 0000 000.0 00.00 00000 00477";
static const char *shortFrame = "01.9 388.9 00745";
// Documented PI41 example; this is not a physical-device capture.
static const char *pi41Frame = "230.0 50.0 230.0 50.0 1000 0900 020 0005 081.6 47.20 00100100";

static const char *const pv2Keys[] = {
    DESCR_PV2_Input_Current, DESCR_PV2_Input_Voltage, DESCR_PV2_Charging_Power,
    DESCR_PV2_Input_Power, DESCR_Battery_SCC2_Volt, DESCR_PV_Total_Input_Power};

static std::string subset(bool pv2)
{
    JsonDocument selected;
    for (JsonPairConst field : JsonObjectConst(liveData))
    {
        bool owned = false;
        for (const char *key : pv2Keys)
            owned = owned || std::strcmp(field.key().c_str(), key) == 0;
        if (owned == pv2)
            selected[field.key().c_str()] = field.value();
    }
    std::string result;
    serializeJson(selected, result);
    return result;
}

static void cycle(protocol_type_t protocol, const char *rating, const char *status,
                  const char *secondTracker, double voltage, double power)
{
    state.clear();
    liveData = state.to<JsonObject>();
    PI_Serial inverter;
    inverter.protocol = protocol;
    inverter.forcedProtocol = protocol;
    inverter.get.raw.qpiri = rating;
    inverter.qpigsReply = status;
    inverter.qpigs2Reply = secondTracker;
    assert(inverter.PIXX_QPIGS());
    JsonDocument firstTracker;
    firstTracker.set(liveData);
    assert(inverter.PIXX_QPIGS2());
    // The PV2 extension must not change any value produced by the PV1 parser.
    for (JsonPairConst field : firstTracker.as<JsonObjectConst>())
        assert(liveData[field.key().c_str()] == field.value());
    assert(std::fabs(liveData[DESCR_PV2_Input_Voltage].as<double>() - voltage) < 0.001);
    assert(std::fabs(liveData[DESCR_PV2_Input_Power].as<double>() - power) < 0.001);
    const auto previousPv2 = subset(true);
    for (unsigned repeat = 0; repeat < 5; ++repeat)
    {
        assert(inverter.PIXX_QPIGS());
        // The next QPIGS does not own these fields and must not remove them.
        assert(subset(true) == previousPv2);
        assert(inverter.PIXX_QPIGS2());
        assert(subset(true) == previousPv2);
    }
    for (const char *failedStatus : {"NAK", "NOA", "ERCRC"})
    {
        inverter.qpigsReply = failedStatus;
        assert(inverter.PIXX_QPIGS() == (std::strcmp(failedStatus, "ERCRC") != 0));
        assert(subset(true) == previousPv2);
    }
    inverter.qpigsReply = status;
    inverter.qpigs2Reply = "ERCRC";
    assert(!inverter.PIXX_QPIGS2());
    assert(subset(true) == previousPv2);
    for (const char *unavailable : {"NAK", "NOA"})
    {
        inverter.qpigs2Reply = unavailable;
        assert(inverter.PIXX_QPIGS2());
        for (const char *key : pv2Keys)
            assert(liveData[key].isNull());
        inverter.qpigs2Reply = secondTracker;
        assert(inverter.PIXX_QPIGS2());
        assert(subset(true) == previousPv2);
    }
}

int main()
{
    cycle(PI30_PIP_GK, gkRating, gkDay, legacy, 79.1, 254);
    cycle(PI30_MAX, maxRating, maxDay, shortFrame, 388.9, 739);
    cycle(PI41, gkRating, gkDay, pi41Frame, 81.6, 408);
    std::puts("PASS: repeated QPIGS/QPIGS2 polling retains PV2 for GK/MAX/PI41, "
              "does not change PV1, and handles NAK/NOA/CRC and recovery.");
}
