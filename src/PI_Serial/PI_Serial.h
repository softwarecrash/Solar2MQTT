#include "SoftwareSerial.h"
// #include "devices/PI30_HS_MS_MSX.h"
#ifndef PI_SERIAL_H
#define PI_SERIAL_H

// time in ms for delay the bms requests, to fast brings connection error
// #define DELAYTINME 100

class PI_Serial
{
public:
    // unsigned int previousTime = 0;
    // byte requestCounter = 0;

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
            //----------------QPIRI--------------------
            float gridRatingVoltage;           // The units is V.
            float gridRatingCurrent;           // The units is A.
            float acOutputRatingVoltage;       // The units is V.
            float acOutputRatingFrquency;      // The units is Hz.
            float acoutputRatingCurrent;       // The unit is A.
            short acOutputRatingApparentPower; // The unit is VA.
            short acOutputRatingActivePower;   // The unit is W.
            float batteryRatingVoltage;        // The units is V.
            float batteryReChargeVoltage;      // The units is V.
            float batteryUnderVoltage;         // The units is V.
            float batteryBulkVoltage;          // The units is V.
            float batteryFloatVoltage;         // The units is V.
            char *batterytype;                 // 0: AGM 1: Flooded 2: User
            short currentMaxAcChargingCurrent; // The units is A.
            short currentMaxChargingCurrent;   // The units is A.
            char *inputVoltageRange;           // 0: Appliance 1: UPS
            char *outputSourcePriority;        // 0: Utility first 1: Solar first 2: SBU first
            char *chargerSourcePriority;       // 0: Utility first 1: Solar first 2: Solar + Utility 3: Only solar charging permitted
            short parallelMaxNumber;           // max parallel inverter / Charger
            char *machineType;                 // 00: Grid tie; 01: Off Grid; 10: Hybrid.
            bool topolgy;                      // 0: transformerless 1: transformer
            char *outputMode;                  // 00: single machine output 01: parallel output 02: Phase 1 of 3 Phase output 03: Phase 2 of 3 Phase output 04: Phase 3 of 3 Phase output
            float batteryReDischargeVoltage;   // The unit is V.
            bool pvOkConditionForParallel;     // 0: As long as one unit of inverters has connect PV, parallel system will consider PV OK; 1: Only All of inverters have connect PV, parallel system will consider PV OK
            bool pvPowerBalance;               // 0: PV input max current will be the max charged current; 1: PV input max power will be the sum of the max charged power and loads power.
            short maxChargingTimeAtCvStage;    // Y is an Integer ranging from 0 to 9. The unit is minute. (Only for PIP-MK )
            char *operationLogik;              // 0: Automatically 1: On-line mode 2: ECO mode (Only for PIP-MK )
        } staticData;
        // grid charge and solar charge array data
        struct
        {
            byte grid[12];
            byte solar[12];
        } chargeValues;
        struct
        {
            //----------------------------QPIGS----------------------------
            float gridVoltage;                                          // The units is V.
            float gridFrequency;                                        // The units is Hz.
            float acOutputVoltage;                                      // The units is V.
            float acOutputFrequency;                                    // The units is Hz.
            short acOutputApparentPower;                                // The units is VA.
            short acOutputActivePower;                                  // The units is W.
            short outputLoadPercent;                                    // The units is %.
            short busVoltage;                                           // The units is V.
            float batteryVoltage;                                       // The units is V.
            short batteryChargingCurrent;                               // The units is A.
            short batteryCapacity;                                      // The units is %.
            short inverterHeatSinkTemperature;                          // The units is ℃
            float pvInputCurrent[4] = {-10000, -10000, -10000, -10000}; // The units is A.
            float pvInputVoltage[4] = {-10000, -10000, -10000, -10000}; // The unitsis V.
            float batteryVoltageFromScc;                                // The units is V.
            short batteryDischargeCurrent;                              // The units is A.
            short batteryVoltageOffsetForFansOn;                        // The unit is 10mV.
            short batteryLoad;                                          // The units is A. - Combined charge and discharge
            short eepromVersion;                                        // version info
            short pvChargingPower;                                      // The unit is watt.

            char *operationMode;
        } variableData;
        struct
        {
            // first part from qpigs (pip sample)
            bool pvOrAcFeedTheLoad;        // PV or AC feed the load, 1:yes,0:no
            bool configurationStatus;      // configuration status: 1:Change 0:unchanged
            bool sccFirmwareVersionChange; // SCC firmware version 1: Updated 0:unchanged
            bool loadStatus;               // Load status: 0: Load off 1:Load on
            bool chargingStatus;           // Charging status( Charging on/off)
            bool sccChargingStatus;        // Charging status( SCC charging on/off)
            bool acChargingStatus;         // Charging status(AC charging on/off)

            bool chargingToFloatingMode; // flag for charging to floating mode
            bool switchOn;               // Switch On
            bool dustproofInstalled;     // flag for dustproof installed

        } deviceStatus;

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
    bool update();

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
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;

private:
    unsigned int soft_tx;
    unsigned int soft_rx;
    unsigned int serialIntfBaud;

    unsigned int protocolType = 100;
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

    void PI30_QPIGS();
    void PI30_QMOD();
    void PI30_QPIRI();

    void PI30_PIP_QPIGS(); // example
    void PI30_PIP_QMOD();  // example
    void PI30_PIP_QPIRI(); // example
};

#endif