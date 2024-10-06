#include "SoftwareSerial.h"
#ifndef PI_SERIAL_H
#define PI_SERIAL_H
#include "vector"
#include <ArduinoJson.h>
#include <modbus/modbus.h>
extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

class PI_Serial
{
public:
    const char *startChar = "(";
    const char *delimiter = " ";
    bool requestStaticData = true;
    byte protocol = NoD;
    bool connection = false;

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
    PI_Serial(int rx, int tx);

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

    enum protocolType
    {
        NoD,
        PI18,
        PI30,
        MODBUS_MUST
    };

private: 
    unsigned int serialIntfBaud;

    unsigned long previousTime = 0;
    unsigned long delayTime = 100;
    byte requestCounter = 0;

    long long int connectionCounter = 0;

    byte qexCounter = 0;
    
    String customCommandBuffer;

    MODBUS *modbus;

    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);

    /**
     * @brief get the crc from a string
     */
    byte getCHK(String data);

    /**
     * @brief function for autodetect the inverter
     * @details ask all modes and sort it to a protocol
     */
    void autoDetect();

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
    SoftwareSerial *my_serialIntf;
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

    bool isModbus();
};

#endif