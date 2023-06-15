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
            //----------------QPI----------------------
            String deviceProtocol;
            //----------------QMN----------------------
            String modelName;

            //----------------QPIRI--------------------
            float gridRatingVoltage = -1;           // The units is V.
            float gridRatingCurrent = -1;           // The units is A.
            float acOutputRatingVoltage = -1;       // The units is V.
            float acOutputRatingFrquency = -1;      // The units is Hz.
            float acoutputRatingCurrent = -1;       // The unit is A.
            short acOutputRatingApparentPower = -1; // The unit is VA.
            short acOutputRatingActivePower = -1;   // The unit is W.
            float batteryRatingVoltage = -1;        // The units is V.
            float batteryReChargeVoltage = -1;      // The units is V.
            float batteryUnderVoltage = -1;         // The units is V.
            float batteryBulkVoltage = -1;          // The units is V.
            float batteryFloatVoltage = -1;         // The units is V.
            const char *batterytype = "";           // 0: AGM 1: Flooded 2: User
            short currentMaxAcChargingCurrent = -1; // The units is A.
            short currentMaxChargingCurrent = -1;   // The units is A.
            const char *inputVoltageRange;          // 0: Appliance 1: UPS
            const char *outputSourcePriority;       // 0: Utility first 1: Solar first 2: SBU first
            const char *chargerSourcePriority;      // 0: Utility first 1: Solar first 2: Solar + Utility 3: Only solar charging permitted
            short parallelMaxNumber = -1;           // max parallel inverter / Charger
            const char *machineType;                // 00: Grid tie; 01: Off Grid; 10: Hybrid.
            bool topolgy;                           // 0: transformerless 1: transformer
            const char *outputMode;                 // 00: single machine output 01: parallel output 02: Phase 1 of 3 Phase output 03: Phase 2 of 3 Phase output 04: Phase 3 of 3 Phase output
            float batteryReDischargeVoltage = -1;   // The unit is V.
            bool pvOkConditionForParallel;          // 0: As long as one unit of inverters has connect PV, parallel system will consider PV OK; 1: Only All of inverters have connect PV, parallel system will consider PV OK
            bool pvPowerBalance;                    // 0: PV input max current will be the max charged current; 1: PV input max power will be the sum of the max charged power and loads power.
            short maxChargingTimeAtCvStage = -1;    // Y is an Integer ranging from 0 to 9. The unit is minute. (Only for PIP-MK )
            const char *operationLogik;             // 0: Automatically 1: On-line mode 2: ECO mode (Only for PIP-MK )
        } staticData;
        // grid charge and solar charge array data
        struct
        {
            byte grid[12];
            byte solar[12];
        } chargeValues;
        struct
        {
            //----------------------------Q1-------------------------------
            // 01 01 00 035 022 023 025 00 00 000 0100 9290 11
            short timeUntilAbsorbCharge = -1;
            short timeUntilfloatCharge = -1;
            short dontKnow0 = -1;
            short trackertemp = -1;
            short InverterTemp = -1;
            short batteryTemp = -1;
            short transformerTemp = -1;
            short dontKnow1 = -1;
            short dontKnow2 = -1;
            short dontKnow3 = -1;
            short fanSpeed = -1;
            short sccChargePower = -1;        // divided by 10
            const char *inverterChargeStatus; // 10:nocharging, 11:bulk stage, 12:absorb, 13:float
            //----------------------------QPIGS----------------------------
            float gridVoltage = -1;                      // The units is V.
            float gridFrequency = -1;                    // The units is Hz.
            float acOutputVoltage = -1;                  // The units is V.
            float acOutputFrequency = -1;                // The units is Hz.
            short acOutputApparentPower = -1;            // The units is VA.
            short acOutputActivePower = -1;              // The units is W.
            short outputLoadPercent = -1;                // The units is %.
            short busVoltage = -1;                       // The units is V.
            float batteryVoltage = -1;                   // The units is V.
            short batteryChargingCurrent = -1;           // The units is A.
            short batteryCapacity = -1;                  // The units is %.
            short inverterHeatSinkTemperature = -1;      // The units is ℃
            float pvInputCurrent[4] = {-1, -1, -1, -1};  // The units is A.
            float pvInputVoltage[4] = {-1, -1, -1, -1};  // The unitsis V.
            float batteryVoltageFromScc = -1;            // The units is V.
            short batteryDischargeCurrent = -1;          // The units is A.
            short batteryVoltageOffsetForFansOn = -1;    // The unit is 10mV.
            short batteryLoad = -1;                      // The units is A. - Combined charge and discharge
            short eepromVersion = -1;                    // version info
            short pvChargingPower[4] = {-1, -1, -1, -1}; // The unit is watt.
            short pvInputWatt[4] = {-1, -1, -1, -1};     // The unit is watt.
            //-------------------extra values from QALL-----------------------
            short pvGenerationDay = -1; // The unit is WH
            short pvGenerationSum = -1; // The unit is KWH

            const char *operationMode = "";
        } variableData;
        struct
        {
            // first part from qpigs (pip sample)
            bool pvOrAcFeedTheLoad;        // PV or AC feed the load, 1:yes,0:no
            bool configurationStatus;      // configuration status: 1:Change 0:unchanged
            bool sccFirmwareVersionChange; // SCC firmware version 1: Updated 0:unchanged
            bool loadStatus;               // Load status: 0: Load off 1:Load on
            bool reservedB3;               // unused bit
            bool chargingStatus;           // Charging status( Charging on/off)
            bool sccChargingStatus;        // Charging status( SCC charging on/off)
            bool acChargingStatus;         // Charging status(AC charging on/off)

            bool chargingToFloatingMode; // flag for charging to floating mode
            bool switchOn;               // Switch On
            bool dustproofInstalled;     // flag for dustproof installed
        } deviceStatus;

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
            String qmod;
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
     * @brief fetching the Variable data
     */
    bool getVariableData();

    /**
     * @brief fetching the Static data
     */
    bool getStaticeData();

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

    struct
    {
        // dynamic
        bool q1 = true;
        bool qpigs = true;
        bool qpigs2 = true;
        bool qall = true;
        bool qmod = true;
        // static
        bool qmn = true;
        bool qpiri = true;
        bool qpi = true;

    } qAvaible;

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
     * @brief Parses out the float
     */
    float getNextFloat(String &command, int &index);

    /**
     * @brief Parses out the long
     */
    long getNextLong(String &command, int &index);

    /**
     * @brief Parses out the long
     */
    int getNextInt(String &command, int &index);

    /**
     * @brief // Gets if the next character is '1'
     */
    bool getNextBit(String &command, int &index);

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
     * @brief accept a achar and get back the operation mode as string
     */
    char *getModeDesc(char mode);

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
    // dynamic requests
    bool PIXX_Q1();
    bool PIXX_QPIGS();
    bool PIXX_QALL();
    bool PIXX_QMOD();
    // static reqeuests
    bool PIXX_QPIRI();
    bool PIXX_QPI();
    bool PIXX_QMN();
};

#endif