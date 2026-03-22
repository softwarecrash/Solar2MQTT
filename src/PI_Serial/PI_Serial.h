#ifndef PI_SERIAL_H
#define PI_SERIAL_H
#include "descriptors.h"
#include <atomic>
#include <HardwareSerial.h>
#include <stdlib.h>
#include <string.h>

#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
//#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>
#include <modbus/modbus.h>

extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

static inline int pi_split_fields(char *buf, char delim, char *fields[], int maxFields)
{
    int count = 0;
    if (!buf || maxFields <= 0)
    {
        return 0;
    }
    char *p = buf;
    while (*p && count < maxFields)
    {
        while (*p == delim)
        {
            ++p;
        }
        if (!*p)
        {
            break;
        }
        fields[count++] = p;
        while (*p && *p != delim)
        {
            ++p;
        }
        if (*p == delim)
        {
            *p = '\0';
            ++p;
        }
    }
    return count;
}

static inline double pi_parse_float2(const char *s)
{
    if (!s || *s == '\0')
    {
        return 0.0;
    }
    char *endptr = nullptr;
    double v = strtod(s, &endptr);
    if (v >= 0.0)
    {
        return (int)(v * 100.0 + 0.5) / 100.0;
    }
    return (int)(v * 100.0 - 0.5) / 100.0;
}

static inline double pi_parse_double(const char *s)
{
    if (!s || *s == '\0')
    {
        return 0.0;
    }
    return strtod(s, nullptr);
}

static inline int pi_round_to_int(double value)
{
    if (value >= 0.0)
    {
        return static_cast<int>(value + 0.5);
    }
    return static_cast<int>(value - 0.5);
}

static inline int pi_compute_power(double voltage, double current)
{
    return pi_round_to_int(voltage * current);
}

template <size_t N>
static inline void pi_clear_json_fields(JsonObject object, const char *const (&fields)[N])
{
    for (size_t i = 0; i < N; ++i)
    {
        if (fields[i] != nullptr && fields[i][0] != '\0')
        {
            object.remove(fields[i]);
        }
    }
}

template <size_t N, size_t M>
static inline void pi_clear_json_field_pairs(JsonObject object, const char *const (&fields)[N][M])
{
    for (size_t i = 0; i < N; ++i)
    {
        if (fields[i][0] != nullptr && fields[i][0][0] != '\0')
        {
            object.remove(fields[i][0]);
        }
    }
}
  
class PI_Serial
{
public:
    const char *startChar = "(";
    const char *delimiter = " ";
    bool requestStaticData = true;
    protocol_type_t protocol = NoD;
    bool connection = false;

    struct
    {
        struct
        {
            // static
            String qpi;
            String qsvfw2;
            String qall;
            String qpiri;
            String qmd;
            String qpibi;
            String qmn;
            String qflag;
            // dynamic
            String q1;
            String qpigs;
            String qpigs2;
            String qmod;
            String qt;
            String qet;
            String qey;
            String qem;
            String qed;
            String qlt;
            String qly;
            String qlm;
            String qld;
            String commandAnswer;
            String qpiws;
        } raw;

    } get;

    struct
    {

    } alarm;

    /**
     * @brief Construct a new PI_Serial object
     *
     * @param serialIntf UART interface BMS is connected to
     */
    PI_Serial(HardwareSerial &serialPort, int rx, int tx);
    ~PI_Serial();

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * @brief Updating the Data from the BMS
     */
    bool loop();

    /**
     * @brief Send custom command to the device
     */
    String sendCommand(String command);

    /**
     * @brief Run a serial TX/RX loopback test.
     */
    bool loopbackTest(String &details);

    void setSuspend(bool enabled);
    bool isSuspended() const;
    bool isBusy() const;
    void setDelayTime(unsigned long value) { delayTime = value; }

    /**
     * @brief
     *
     */
    bool sendCustomCommand();

    /**
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;

 

private: 
    unsigned int serialIntfBaud;
    std::atomic<uint8_t> busyCount{0};
    std::atomic_bool suspendSerial{false};
    std::atomic_bool abortAutoDetect{false};
    bool cycleBackupActive = false;

    unsigned long previousTime = 0;
    unsigned long delayTime = 100;
    unsigned long nextDetectAt = 0;
    unsigned long lastSuccessfulDynamicCycleAt = 0;
    byte requestCounter = 0;

    long long int connectionCounter = 0;

    byte qexCounter = 0;
    
    String customCommandBuffer;
    JsonDocument cycleLiveBackup;
    JsonDocument cycleStaticBackup;

    MODBUS *modbus = nullptr;

    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);
    uint16_t getCRC(const char *data, size_t len);

    /**
     * @brief get the crc from a string
     */
    byte getCHK(String data);
    byte getCHK(const char *data, size_t len);

    /**
     * @brief function for autodetect the inverter
     * @details ask all modes and sort it to a protocol
     */
    void autoDetect();

    void beginCycleBackup();
    void restoreCycleBackup();
    void clearCycleBackup();
    void markSuccessfulDynamicCycle();
    void refineProtocol();
    bool requestUnsupportedPiStatic();
    bool requestUnsupportedPiDynamic();
    bool requestAndStoreRaw(const char *command, String &target, bool &hadSuccessfulReply);
    bool isValidResponse(const String &response) const;
    void logStaticSummary() const;
    void logDynamicSummary() const;

    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    String requestData(String command);

    /**
     * @brief accept a achar and get back the operation mode as string
     */
    char *getModeDesc(char mode);

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    HardwareSerial *my_serialIntf;
    int _rxPin;
    int _txPin;
    // dynamic requests
    bool PIXX_Q1();
    bool PIXX_QPIGS();
    bool PIXX_QPIGS2();
    bool PIXX_QMOD();
    bool PIXX_QEX();
    bool PIXX_QPIWS();
    // static reqeuests
    bool PIXX_QPIRI();
    bool PIXX_QPI();
    bool PIXX_QMN();
    bool PIXX_QFLAG();

    bool isModbus();

    static bool checkQFLAG(const String& flags, char symbol);
};

#endif
