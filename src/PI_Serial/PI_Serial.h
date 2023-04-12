#include "SoftwareSerial.h"
#ifndef PI_SERIAL_H
#define PI_SERIAL_H


#ifndef SERIAL_TX
#define SERIAL_TX 13
#endif
#ifndef SERIAL_RX
#define SERIAL_RX 12
#endif

//time in ms for delay the bms requests, to fast brings connection error
#define DELAYTINME 100

// DON'T edit DEBUG here, edit build_type in platformio.ini !!!
#ifdef isDEBUG
#define DEBUG_SERIAL Serial
#endif

#ifdef DEBUG_SERIAL
//make it better like
//https://stackoverflow.com/questions/28931195/way-to-toggle-debugging-code-on-and-off
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

enum protocolType
{
  PI00,
  PI16,
  PI17,
  PI18,
  PI30_MAX,// current implementet protocol
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
    unsigned int PI_Serial::autoDetect();

    /**
     * @brief Updating the Data from the BMS
     */
    bool update();

    /**
     * @brief Gets Voltage, Current, and SOC measurements from the BMS
     * @return True on successful aquisition, false otherwise
     */
    bool getPackMeasurements();

    /**
     * @brief Gets the pack temperature from the min and max of all the available temperature sensors
     * @details Populates tempMax, tempMax, and tempAverage in the "get" struct
     * @return True on successful aquisition, false otherwise
     */
    bool getPackTemp();

    /**
     * @brief Returns the highest and lowest individual cell voltage, and which cell is highest/lowest
     * @details Voltages are returned as floats with milliVolt precision (3 decimal places)
     * @return True on successful aquisition, false otherwise
     */
    bool getMinMaxCellVoltage();

    /**
     * @brief Get the general Status Info
     *
     */
    bool getStatusInfo();

    /**
     * @brief Get Cell Voltages
     *
     */
    bool getCellVoltages();

    /**
     * @brief   Each temperature accounts for 1 byte, according to the
                actual number of temperature send, the maximum 21
                byte, send in 3 frames
                Byte0:frame number, starting at 0
                Byte1~byte7:cell temperature(40 Offset ,℃)
     *
     */
    bool getCellTemperature();

    /**
     * @brief   0： Closed 1： Open
                Bit0: Cell 1 balance state
                ...
                Bit47:Cell 48 balance state
                Bit48~Bit63：reserved
     *
     */
    bool getCellBalanceState();

    /**
     * @brief Get the Failure Codes
     *
     */
    bool getFailureCodes();

    /**
     * @brief
     * set the Discharging MOS State
     */
    bool setDischargeMOS(bool sw);

    /**
     * @brief set the Charging MOS State
     *
     */
    bool setChargeMOS(bool sw);

    /**
     * @brief set the SOC
     *
     */
    bool setSOC(float sw);

    /**
     * @brief Read the charge and discharge MOS States
     *
     */
    bool getDischargeChargeMosStatus();

    /**
     * @brief Reseting The BMS
     * @details Reseting the BMS and let it restart
     */
    bool setBmsReset();

    /**
     * @brief return the state of connection to the BMS
     * @details returns the following value for different connection state
     * -3 - could not open serial port
     * -2 - no data recived or wrong crc, check connection
     * -1 - working and collecting data, please wait
     *  0 - All data recived with correct crc, idleing
     */
    int getState();

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