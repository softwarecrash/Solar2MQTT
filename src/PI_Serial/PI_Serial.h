#include "SoftwareSerial.h"

#ifndef PI_SERIAL_H
#define PI_SERIAL_H

#ifndef SERIAL_TX
#define SERIAL_TX 13
#endif
#ifndef SERIAL_RX
#define SERIAL_RX 12
#endif

// time in ms for delay the bms requests, to fast brings connection error
#define DELAYTINME 100

// DON'T edit DEBUG here, edit build_type in platformio.ini !!!
#ifdef isDEBUG
#define DEBUG_SERIAL Serial
#endif

#ifdef DEBUG_SERIAL
// make it better like
// https://stackoverflow.com/questions/28931195/way-to-toggle-debugging-code-on-and-off
#define BMS_DEBUG_BEGIN(...) DEBUG_SERIAL.begin(__VA_ARGS__)
#define BMS_DEBUG_PRINT(...) DEBUG_SERIAL.print(__VA_ARGS__)
#define BMS_DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#define BMS_DEBUG_WRITE(...) DEBUG_SERIAL.write(__VA_ARGS__)
#define BMS_DEBUG_PRINTLN(...) DEBUG_SERIAL.println(__VA_ARGS__)
#else
#undef BMS_DEBUG_BEGIN
#undef BMS_DEBUG_PRINT
#undef BMS_DEBUG_PRINTF
#undef BMS_DEBUG_WRITE
#undef BMS_DEBUG_PRINTLN
#define BMS_DEBBUG_BEGIN(...)
#define BMS_DEBUG_PRINT(...)
#define BMS_DEBUG_PRINTF(...)
#define BMS_DEBUG_WRITE(...)
#define BMS_DEBUG_PRINTLN(...)
#endif

class PI_Serial
{
public:
    unsigned int previousTime = 0;
    byte requestCounter = 0;
    int soft_tx;
    int soft_rx;
    const char *startChar = "("; //move later to changeable

    enum protocolType
    {
        PI00,
        PI16,
        PI17,
        PI18,
        PI30_MAX, // current implementet protocol
        PI30_REVO,
        PI30_C,
        PI30_HS_MS_MSX,
        PI30_PIP,
        PI34,
        PI41,
    };

    enum COMMAND
    {

    };

    struct
    {

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
     * @brief function for autodetect the inverter
     * @details ask all modes and sort it to a protocol
     */
    unsigned int autoDetect();

    /**
     * @brief set global the protocol
     */
    bool setProtocol(int protocolID);

    /**
     * @brief Updating the Data from the BMS
     */
    bool update();


    /**
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;

private:
    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    void requestData(String command);

    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);

    /**
     * @brief append the calcualted crc to the given string and return it
     */
    String appendCRC(String data);

    /**
     * @brief Clear all data from the Get struct
     * @details when wrong or missing data comes in it need sto be cleared
     */
    void clearGet();

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    SoftwareSerial *my_serialIntf;
};

#endif