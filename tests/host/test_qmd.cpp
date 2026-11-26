#include <ArduinoJson.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "descriptors.h"

// Native String adapter. Model parsing, reply normalization and JSON use production code.
class String
{
public:
    String() = default;
    String(const char *input) : value(input ? input : "") {}
    explicit String(std::string input) : value(input) {}

    bool isEmpty() const { return value.empty(); }
    size_t length() const { return value.size(); }
    char operator[](size_t position) const { return value[position]; }
    char charAt(size_t position) const { return (*this)[position]; }
    void reserve(size_t capacity) { value.reserve(capacity); }
    void operator+=(char c) { value += c; }
    bool operator==(const char *other) const { return value == other; }
    const char *c_str() const { return value.c_str(); }
    String substring(size_t first, size_t end) const { return String(value.substr(first, end - first)); }
    void toCharArray(char *buffer, size_t size) const { buffer[value.copy(buffer, size - 1)] = '\0'; }
    void trim()
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            value.clear();
        else
            value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    }

private:
    std::string value;
};

namespace ArduinoJson {
template <> struct Converter<String>
{
    static void toJson(const String &value, JsonVariant out) { out.set(value.c_str()); }
};
}

#include "qmd_helpers.h"

JsonDocument state;
JsonObject staticData;

class PI_Serial
{
public:
    protocol_type_t protocol = PI30_PIP_GK;
    struct { struct { String qmn, qmd; } raw; } get;
    String qmn = "NAK";
    String qmd = "NAK";
    std::vector<std::string> commands;

    String requestData(const char *command)
    {
        commands.emplace_back(command);
        if (std::strcmp(command, "QMN") == 0)
            return qmn;
        assert(std::strcmp(command, "QMD") == 0);
        return qmd;
    }

    bool PIXX_QMN();
};
#include "PI_Serial/QMN.h"

static void reset()
{
    state.clear();
    staticData = state.to<JsonObject>();
    staticData[DESCR_AC_Out_Rating_Active_Power] = 4000;
    staticData[DESCR_Battery_Rating_Voltage] = 48;
}

static void expectModel(const char *model)
{
    assert(staticData[DESCR_Device_Model] == model);
    assert(staticData[DESCR_AC_Out_Rating_Active_Power] == 4000);
    assert(staticData[DESCR_Battery_Rating_Voltage] == 48);
}

int main()
{
    // The recorded legacy reply already used in the model regression.
    const char *captured = "#####INVERTEX5K ###5000 99 1/1 230 230 04 12.0";
    const std::vector<std::string> qmnOnly{"QMN"};
    const std::vector<std::string> fallback{"QMN", "QMD"};

    for (auto protocol : {PI30, PI30_UNKNOWN, PI30_MAX, PI30_PIP_GK, PI41, PI30_REVO})
    {
        reset();
        PI_Serial driver;
        driver.protocol = protocol;
        driver.qmd = captured;
        assert(driver.PIXX_QMN());
        assert(driver.commands == fallback);
        expectModel("INVERTEX5K");
        assert(driver.get.raw.qmn == "NAK" && driver.get.raw.qmd == captured);

        driver.commands.clear();
        driver.qmn = "MKS2-11000";
        assert(driver.PIXX_QMN());
        assert(driver.commands == qmnOnly);
        expectModel("MKS2-11000");
        assert(driver.get.raw.qmd.isEmpty());
    }

    for (const char *reply : {"     INVERTEX5K    5000 99 1/1 230 230 04 12.0",
                              "INVERTEX5K     ###5000 99 1/1 230 230 04 12.0",
                              "###INVERTEX5K## ###5000 99 1/1 230 230 04 12.0",
                              "     INVERTER 5K    5000 99 1/1 230 230 04 12.0"})
    {
        reset();
        PI_Serial driver;
        driver.qmd = sanitizePiReplyText(reply);
        assert(driver.PIXX_QMN() && driver.commands == fallback);
        expectModel(std::strstr(reply, "INVERTER 5K") ? "INVERTER 5K" : "INVERTEX5K");
    }

    for (const char *reply : {"NAK", "NOA", "ERCRC", "", "INVERTEX5K ###5000",
                              "############### ###5000 99 1/1 230 230 04 12.0",
                              "#####INVER#EX5K ###5000 99 1/1 230 230 04 12.0",
                              "INVERTEX5K ####### 99 1/1 230 230 04 12.0",
                              "INVERTEX5K ###5000 99 111 230 230 04 12.0",
                              "INVERTEX5K ###5000 99 1/1 230 230 04 1200",
                              "INVERTEX5K ###5000 99 1/1 230 230 04 12.0 extra",
                              "ABCDEFGHIJKLMNOP ###5000 99 1/1 230 230 04 12.0"})
    {
        reset();
        PI_Serial driver;
        driver.qmd = reply;
        assert(driver.PIXX_QMN() && driver.commands == fallback);
        assert(staticData[DESCR_Device_Model].isNull() && driver.get.raw.qmd == reply);
        driver.commands.clear();
        driver.qmd = captured;
        assert(driver.PIXX_QMN() && driver.commands == fallback);
        expectModel("INVERTEX5K");
        driver.commands.clear();
        driver.qmd = reply;
        assert(driver.PIXX_QMN() && driver.commands == fallback);
        expectModel("INVERTEX5K");
        assert(driver.get.raw.qmd == reply);
    }

    for (const char *reply : {"NOA", "ERCRC", "", "bad"})
    {
        reset();
        PI_Serial driver;
        driver.qmn = reply;
        driver.qmd = captured;
        assert(driver.PIXX_QMN() == (std::strcmp(reply, "ERCRC") != 0));
        assert(driver.commands == qmnOnly && staticData[DESCR_Device_Model].isNull());
    }

    for (auto protocol : {PI16, PI18, NoD})
    {
        reset();
        PI_Serial driver;
        driver.protocol = protocol;
        assert(driver.PIXX_QMN() == (protocol != NoD));
        assert(driver.commands.empty());
    }
    std::puts("QMD: model fallback, original ratings, QMN priority, padding, failures and recovery passed");
}
