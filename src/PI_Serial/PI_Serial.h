#include "SoftwareSerial.h"
//#include "devices/PI30_HS_MS_MSX.h"
#ifndef PI_SERIAL_H
#define PI_SERIAL_H

// time in ms for delay the bms requests, to fast brings connection error
#define DELAYTINME 100


class PI_Serial
{
public:
    unsigned int previousTime = 0;
    byte requestCounter = 0;
    int soft_tx;
    int soft_rx;
    

    enum protocolType
    {
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
            float gridRatingV;   // Grid rating voltage
            float gridRatingA;   // Grid rating current
            float acOutRatingV;  // AC output rating voltage
            float acOutRatingHz; // AC output rating frequency
            float acOutRatingA;  // AC output rating current
            float acOutRatungVA; // AC output rating apparent power
            float acOutRatingW;  // AC output rating active power
            float battRatingV;   // Battery rating voltage
            float battreChargeV; // Battery re-charge voltage
            float battUnderV;    // Battery under voltage
            float battBulkV;     // Battery bulk voltage
            float battFloatV;    // Battery float voltage
            String battType;     // Battery type
            byte battMaxAcChrgA; // Current max AC charging current
            byte battMaxChrgA;   // Current max charging current
        } staticData;
        // grid charge and solar charge array data
        struct
        {
            byte grid[12];
            byte solar[12];
        } chargeValues;
        struct
        {
            // QPIGS
            float gridV;
            float gridHz;
            float acOutV;
            float acOutHz;
            short acOutVa;
            short acOutW;
            byte acOutPercent;
            short busV;
            float battV;
            byte battChargeA;
            byte battPercent;
            float heatSinkDegC;
            byte solarA;
            byte solarV;
            float sccBattV;
            int batteryLoad; // Value is Ampere
            short solarW;

            float addSbuPriorityVersion;
            float isConfigChanged;
            float isSccFirmwareUpdated;

            float battVoltageToSteadyWhileCharging;
            float chargingStatus;
            float reservedY;
            float reservedZ;
            float reservedAA;
            float reservedBB;
            // QMOD
            String operationMode;
        } variableData;

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
     * @brief fetching the QPIGS Data
     */
    bool getVariableData();

    /**
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;

private:
    unsigned int protocolType;
    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);

    /**
     * @brief append the calcualted crc to the given string and return it
     */
    String appendCRC(String data);

    /**
     * @brief Parses out the float
     */
    float getNextFloat(String &command, int &index);

    /**
     * @brief Parses out the long
     */
    long getNextLong(String &command, int &index);
    /**
     * @brief // Gets if the next character is '1'
     */
    bool getNextBit(String &command, int &index);

    /**
     * @brief Sends a complete packet with the specified command
     * @details calculates the checksum and sends the command over the specified serial connection
     */
    String requestData(String command);

    /**
     * @brief accept a achar and get back the operation mode as string
     */
    String getModeDesc(char mode);

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




    void PI30_HS_MS_MSX_QPIGS();
    void PI30_HS_MS_MSX_QMOD();
};

#endif