#include "SoftwareSerial.h"
#ifndef PI_SERIAL_H
#define PI_SERIAL_H

// DON'T edit DEBUG here, edit build_type in platformio.ini !!!
#ifdef isDEBUG
#define DEBUG_SERIAL Serial
#include <WebSerialLite.h>
#endif

#ifdef DEBUG_SERIAL

#define PI_DEBUG_BEGIN(...) DEBUG_SERIAL.begin(__VA_ARGS__)
#define PI_DEBUG_PRINT(...) DEBUG_SERIAL.print(__VA_ARGS__)
#define PI_DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#define PI_DEBUG_WRITE(...) DEBUG_SERIAL.write(__VA_ARGS__)
#define PI_DEBUG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#define PI_DEBUG_WEB(...) WebSerial.print(__VA_ARGS__)
#define PI_DEBUG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#else
#undef PI_DEBUG_BEGIN
#undef PI_DEBUG_PRINT
#undef PI_DEBUG_PRINTF
#undef PI_DEBUG_WRITE
#undef PI_DEBUG_PRINTLN
#undef PI_DEBUG_WEB
#undef PI_DEBUG_WEBLN
#define PI_DEBBUG_BEGIN(...)
#define PI_DEBUG_PRINT(...)
#define PI_DEBUG_PRINTF(...)
#define PI_DEBUG_WRITE(...)
#define PI_DEBUG_PRINTLN(...)
#define PI_DEBUG_WEB(...)
#define PI_DEBUG_WEBLN(...)
#endif

#include <ArduinoJson.h>
// extern DynamicJsonDocument Json;
extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

class PI_Serial
{
public:
    const char *startChar = "("; // move later to changeable
    bool requestStaticData = true;

    enum protocolType
    {
        PIXX,
        PI00,
        PI16,
        PI17,
        PI18,
        PI30_MAX,
        PI30_REVO,
        PI30_C,
        PI30_HS_MS_MSX, // current implementet protocol
        PI30_PIP,
        PI34,
        PI41,
    };
    struct
    {
        struct
        {
            // static
            String qpi;
            String qall;
            String qpiri;
            String qmn;
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
    PI_Serial(int rx, int tx);

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * @brief set global the protocol
     */
    bool setProtocol(int protocolID);

    /**
     * @brief Updating the Data from the BMS
     */
    bool loop();

    /**
     * @brief Send custom command to the device
     */
    String sendCommand(String command);

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
    unsigned int soft_tx;
    unsigned int soft_rx;
    unsigned int serialIntfBaud;

    unsigned int previousTime = 0;
    unsigned int delayTime = 50;
    byte requestCounter = 0;
    String customCommandBuffer;

    unsigned int protocolType = 100;

    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);

    /**
     * @brief get the crc from a string
     */
    byte getCHK(String data);

    /**
     * @brief append the calcualted crc to the given string and return it
     */
    String appendCRC(String data);

    /**
     * @brief append the calcualted crc to the given string and return it
     */
    String appendCHK(String data);

    /**
     * @brief function for autodetect the inverter
     * @details ask all modes and sort it to a protocol
     */
    unsigned int autoDetect();

    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    String requestData(String command);

    /**
     * @brief Clear all data from the Get struct
     * @details when wrong or missing data comes in it need sto be cleared
     */
    void clearGet();

    /**
     * @brief accept a achar and get back the operation mode as string
     */
    char *getModeDesc(char mode);

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    SoftwareSerial *my_serialIntf;
    // dynamic requests
    bool PIXX_Q1();
    bool PIXX_QPIGS();
    bool PIXX_QPIGS2();
    bool PIXX_QALL();
    bool PIXX_QMOD();
    bool PIXX_QEX();
    // static reqeuests
    bool PIXX_QPIRI();
    bool PIXX_QPI();
    bool PIXX_QMN();
};

#endif