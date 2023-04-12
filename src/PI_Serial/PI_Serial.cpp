#include "PI_Serial.h"
SoftwareSerial myPort;

//----------------------------------------------------------------------
// Public Functions
//----------------------------------------------------------------------

PI_Serial::PI_Serial(int rx, int tx)
{
    // SoftwareSerial myPort;
    // this->my_serialIntf = &serial_peripheral;
    // soft_rx = rx;
    // soft_tx = tx;
    this->my_serialIntf = &myPort;
}

bool PI_Serial::Init()
{

    // Initialize debug serial interface
    // BMS_DEBUG_BEGIN(9600);

    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        BMS_DEBUG_PRINTLN("<PI SERIAL> ERROR: No serial peripheral specificed!");
        get.connectionState = -3;
        return false;
    }

    this->my_serialIntf->begin(2400, SWSERIAL_8N1, SERIAL_RX, SERIAL_TX, false);
    memset(this->my_txBuffer, 0x00, XFER_BUFFER_LENGTH);
    clearGet();
    return true;
}

unsigned int PI_Serial::autoDetect() //function for autodetect the inverter type
{
    /*
        QPI abfragen
        wenn antwort nicht NAK dann schauen welche nummer, anhand der nummer zuordnen
        wenn 30 dann QPIGS, QPIRI abfragen und anhand der längen das protokoll zuordnen  
        wenn NAK dann??
        wenn keine antwort, dann ist es ein protokoll mit anderen vorzeichen, dann änderung der preampel und erneut versuchen.
    */
}

bool PI_Serial::setProtocol(int protocolID)
{
    protocolID = PI30MAX;
    return true;
}
/*

bool PI_Serial::update()
{
    get.connectionState = -1;
    //  Call all get___() functions to populate all members of the "get" struct
    if (millis() - previousTime >= DELAYTINME && requestCounter == 0)
    {
        previousTime = millis();
        if (!getPackMeasurements())
        {
            get.connectionState = -2;
            return false; // 0x90
        }
        else
        {
            requestCounter = 1;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 1)
    {
        previousTime = millis();
        if (!getMinMaxCellVoltage())
        {
            get.connectionState = -2;
            return false; // 0x91
        }
        else
        {
            requestCounter = 2;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 2)
    {
        previousTime = millis();
        if (!getPackTemp())
        {
            get.connectionState = -2;
            return false; // 0x92
        }
        else
        {
            requestCounter = 3;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 3)
    {
        previousTime = millis();
        if (!getDischargeChargeMosStatus())
        {
            get.connectionState = -2;
            return false; // 0x93
        }
        else
        {
            requestCounter = 4;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 4)
    {
        previousTime = millis();
        if (!getStatusInfo())
        {
            get.connectionState = -2;
            return false; // 0x94
        }
        else
        {
            requestCounter = 5;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 5)
    {
        previousTime = millis();
        if (!getCellVoltages())
        {
            get.connectionState = -2;
            return false; // 0x95
        }
        else
        {
            requestCounter = 6;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 6)
    {
        previousTime = millis();
        if (!getCellTemperature())
        {
            get.connectionState = -2;
            return false; // 0x96
        }
        else
        {
            requestCounter = 7;
        }
    }

    if (millis() - previousTime >= DELAYTINME && requestCounter == 7)
    {
        previousTime = millis();
        if (!getCellBalanceState())
        {
            get.connectionState = -2;
            return false; // 0x97
        }
        else
        {
            // requestCounter = 8;
            get.connectionState = 0;
            requestCounter = 0;

            // for testing, a callback function to inform another function outside that data avaible
            requestCallback();
        }
    }
    /*
        if (millis() - previousTime >= DELAYTINME && requestCounter == 8)
        {
            previousTime = millis();
            if (!getFailureCodes())
            {
                get.connectionState = -2;
                return false; // 0x98
            }
            else
            {
                get.connectionState = 0;
                requestCounter = 0;
            }
        }
    */
    return true;
}

bool PI_Serial::getPackMeasurements() // 0x90
{
    this->sendCommand(COMMAND::VOUT_IOUT_SOC);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, V, I, & SOC values won't be modified!\n");
        clearGet();
        return false;
    }
    else if (((float)(((this->my_rxBuffer[8] << 8) | this->my_rxBuffer[9]) - 30000) / 10.0f) == -3000)
    {
        clearGet();
        return false;
    }

    // Pull the relevent values out of the buffer
    get.packVoltage = ((float)((this->my_rxBuffer[4] << 8) | this->my_rxBuffer[5]) / 10.0f);
    // The current measurement is given with a 30000 unit offset (see /docs/)
    get.packCurrent = ((float)(((this->my_rxBuffer[8] << 8) | this->my_rxBuffer[9]) - 30000) / 10.0f);
    get.packSOC = ((float)((this->my_rxBuffer[10] << 8) | this->my_rxBuffer[11]) / 10.0f);
    BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> " + (String)get.packVoltage + "V, " + (String)get.packCurrent + "A, " + (String)get.packSOC + "SOC");
    return true;
}

bool PI_Serial::getMinMaxCellVoltage() // 0x91
{
    this->sendCommand(COMMAND::MIN_MAX_CELL_VOLTAGE);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, min/max cell values won't be modified!\n");
        return false;
    }

    get.maxCellmV = (float)((this->my_rxBuffer[4] << 8) | this->my_rxBuffer[5]);
    get.maxCellVNum = this->my_rxBuffer[6];
    get.minCellmV = (float)((this->my_rxBuffer[7] << 8) | this->my_rxBuffer[8]);
    get.minCellVNum = this->my_rxBuffer[9];
    get.cellDiff = (get.maxCellmV - get.minCellmV);

    return true;
}

bool PI_Serial::getPackTemp() // 0x92
{
    this->sendCommand(COMMAND::MIN_MAX_TEMPERATURE);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Temp values won't be modified!\n");
        return false;
    }
    get.tempAverage = ((this->my_rxBuffer[4] - 40) + (this->my_rxBuffer[6] - 40)) / 2;

    return true;
}

bool PI_Serial::getDischargeChargeMosStatus() // 0x93
{
    this->sendCommand(COMMAND::DISCHARGE_CHARGE_MOS_STATUS);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Charge / discharge mos Status won't be modified!\n");
        return false;
    }

    switch (this->my_rxBuffer[4])
    {
    case 0:
        get.chargeDischargeStatus = "Stationary";
        break;
    case 1:
        get.chargeDischargeStatus = "Charge";
        break;
    case 2:
        get.chargeDischargeStatus = "Discharge";
        break;
    }

    get.chargeFetState = this->my_rxBuffer[5];
    get.disChargeFetState = this->my_rxBuffer[6];
    get.bmsHeartBeat = this->my_rxBuffer[7];
    get.resCapacitymAh = ((uint32_t)my_rxBuffer[8] << 0x18) | ((uint32_t)my_rxBuffer[9] << 0x10) | ((uint32_t)my_rxBuffer[10] << 0x08) | (uint32_t)my_rxBuffer[11];

    return true;
}

bool PI_Serial::getStatusInfo() // 0x94
{
    this->sendCommand(COMMAND::STATUS_INFO);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Status info won't be modified!\n");
        return false;
    }

    get.numberOfCells = this->my_rxBuffer[4];
    get.numOfTempSensors = this->my_rxBuffer[5];
    get.chargeState = this->my_rxBuffer[6];
    get.loadState = this->my_rxBuffer[7];

    // Parse the 8 bits into 8 booleans that represent the states of the Digital IO
    for (size_t i = 0; i < 8; i++)
    {
        get.dIO[i] = bitRead(this->my_rxBuffer[8], i);
    }

    get.bmsCycles = ((uint16_t)this->my_rxBuffer[9] << 0x08) | (uint16_t)this->my_rxBuffer[10];

    return true;
}

bool PI_Serial::getCellVoltages() // 0x95
{
    unsigned int cellNo = 0; // start with cell no. 1

    // Check to make sure we have a valid number of cells
    if (get.numberOfCells < MIN_NUMBER_CELLS && get.numberOfCells >= MAX_NUMBER_CELLS)
    {
        return false;
    }

    this->sendCommand(COMMAND::CELL_VOLTAGES);

    // for (size_t i = 0; i <= ceil(get.numberOfCells / 3); i++)
    for (size_t i = 0; i < (unsigned int)ceil(get.numberOfCells / 3.0); i++) // test for bug #67
    {
        if (!this->receiveBytes())
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Cell Voltages won't be modified!\n");

            // do // clear all incoming serial to avoid data collision
            // {
            // } while (this->my_serialIntf->read() > 0);

            return false;
        }

        for (size_t i = 0; i < 3; i++)
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Frame No.: " + (String)this->my_rxBuffer[4]);
            BMS_DEBUG_PRINTLN(" Cell No: " + (String)(cellNo + 1) + ". " + (String)((this->my_rxBuffer[5 + i + i] << 8) | this->my_rxBuffer[6 + i + i]) + "mV");
            get.cellVmV[cellNo] = (this->my_rxBuffer[5 + i + i] << 8) | this->my_rxBuffer[6 + i + i];
            cellNo++;
            if (cellNo >= get.numberOfCells)
                break;
        }
    }

    return true;
}

bool PI_Serial::getCellTemperature() // 0x96
{
    unsigned int sensorNo = 0;

    // Check to make sure we have a valid number of temp sensors
    if ((get.numOfTempSensors < MIN_NUMBER_TEMP_SENSORS) && (get.numOfTempSensors >= MAX_NUMBER_TEMP_SENSORS))
    {
        return false;
    }

    this->sendCommand(COMMAND::CELL_TEMPERATURE);

    for (size_t i = 0; i < ceil(get.numOfTempSensors / 7.0); i++)
    {

        if (!this->receiveBytes())
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Cell Temperatures won't be modified!\n");
            return false;
        }

        for (size_t i = 0; i < 7; i++)
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Frame No.: " + (String)this->my_rxBuffer[4]);
            BMS_DEBUG_PRINTLN(" Sensor No: " + (String)(sensorNo + 1) + ". " + String(this->my_rxBuffer[5 + i] - 40) + "C");
            get.cellTemperature[sensorNo] = (this->my_rxBuffer[5 + i] - 40);
            sensorNo++;
            if (sensorNo >= get.numOfTempSensors)
                break;
        }
    }
    return true;
}

bool PI_Serial::getCellBalanceState() // 0x97
{
    int cellBalance = 0;
    int cellBit = 0;

    // Check to make sure we have a valid number of cells
    if (get.numberOfCells < MIN_NUMBER_CELLS && get.numberOfCells >= MAX_NUMBER_CELLS)
    {
        return false;
    }

    this->sendCommand(COMMAND::CELL_BALANCE_STATE);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Receive failed, Cell Balance State won't be modified!\n");
        return false;
    }

    // We expect 6 bytes response for this command
    for (size_t i = 0; i < 6; i++)
    {
        // For each bit in the byte, pull out the cell balance state boolean
        for (size_t j = 0; j < 8; j++)
        {
            get.cellBalanceState[cellBit] = bitRead(this->my_rxBuffer[i + 4], j);
            cellBit++;
            if (bitRead(this->my_rxBuffer[i + 4], j))
            {
                cellBalance++;
            }
            if (cellBit >= 47)
            {
                break;
            }
        }
    }

    BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Cell Balance State: ");
    for (size_t i = 0; i < get.numberOfCells; i++)
    {
        BMS_DEBUG_PRINT(get.cellBalanceState[i]);
    }
    BMS_DEBUG_PRINTLN();

    if (cellBalance > 0)
    {
        get.cellBalanceActive = true;
    }
    else
    {
        get.cellBalanceActive = false;
    }

    return true;
}

bool PI_Serial::getFailureCodes() // 0x98
{
    this->sendCommand(COMMAND::FAILURE_CODES);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Receive failed, Failure Flags won't be modified!\n");
        return false;
    }

    /* 0x00 */
    alarm.levelOneCellVoltageTooHigh = bitRead(this->my_rxBuffer[4], 0);
    alarm.levelTwoCellVoltageTooHigh = bitRead(this->my_rxBuffer[4], 1);
    alarm.levelOneCellVoltageTooLow = bitRead(this->my_rxBuffer[4], 2);
    alarm.levelTwoCellVoltageTooLow = bitRead(this->my_rxBuffer[4], 3);
    alarm.levelOnePackVoltageTooHigh = bitRead(this->my_rxBuffer[4], 4);
    alarm.levelTwoPackVoltageTooHigh = bitRead(this->my_rxBuffer[4], 5);
    alarm.levelOnePackVoltageTooLow = bitRead(this->my_rxBuffer[4], 6);
    alarm.levelTwoPackVoltageTooLow = bitRead(this->my_rxBuffer[4], 7);

    /* 0x01 */
    alarm.levelOneChargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoChargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneChargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoChargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneDischargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoDischargeTempTooHigh = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelOneDischargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);
    alarm.levelTwoDischargeTempTooLow = bitRead(this->my_rxBuffer[5], 1);

    /* 0x02 */
    alarm.levelOneChargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 0);
    alarm.levelTwoChargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 1);
    alarm.levelOneDischargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 2);
    alarm.levelTwoDischargeCurrentTooHigh = bitRead(this->my_rxBuffer[6], 3);
    alarm.levelOneStateOfChargeTooHigh = bitRead(this->my_rxBuffer[6], 4);
    alarm.levelTwoStateOfChargeTooHigh = bitRead(this->my_rxBuffer[6], 5);
    alarm.levelOneStateOfChargeTooLow = bitRead(this->my_rxBuffer[6], 6);
    alarm.levelTwoStateOfChargeTooLow = bitRead(this->my_rxBuffer[6], 7);

    /* 0x03 */
    alarm.levelOneCellVoltageDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 0);
    alarm.levelTwoCellVoltageDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 1);
    alarm.levelOneTempSensorDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 2);
    alarm.levelTwoTempSensorDifferenceTooHigh = bitRead(this->my_rxBuffer[7], 3);

    /* 0x04 */
    alarm.chargeFETTemperatureTooHigh = bitRead(this->my_rxBuffer[8], 0);
    alarm.dischargeFETTemperatureTooHigh = bitRead(this->my_rxBuffer[8], 1);
    alarm.failureOfChargeFETTemperatureSensor = bitRead(this->my_rxBuffer[8], 2);
    alarm.failureOfDischargeFETTemperatureSensor = bitRead(this->my_rxBuffer[8], 3);
    alarm.failureOfChargeFETAdhesion = bitRead(this->my_rxBuffer[8], 4);
    alarm.failureOfDischargeFETAdhesion = bitRead(this->my_rxBuffer[8], 5);
    alarm.failureOfChargeFETTBreaker = bitRead(this->my_rxBuffer[8], 6);
    alarm.failureOfDischargeFETBreaker = bitRead(this->my_rxBuffer[8], 7);

    /* 0x05 */
    alarm.failureOfAFEAcquisitionModule = bitRead(this->my_rxBuffer[9], 0);
    alarm.failureOfVoltageSensorModule = bitRead(this->my_rxBuffer[9], 1);
    alarm.failureOfTemperatureSensorModule = bitRead(this->my_rxBuffer[9], 2);
    alarm.failureOfEEPROMStorageModule = bitRead(this->my_rxBuffer[9], 3);
    alarm.failureOfRealtimeClockModule = bitRead(this->my_rxBuffer[9], 4);
    alarm.failureOfPrechargeModule = bitRead(this->my_rxBuffer[9], 5);
    alarm.failureOfVehicleCommunicationModule = bitRead(this->my_rxBuffer[9], 6);
    alarm.failureOfIntranetCommunicationModule = bitRead(this->my_rxBuffer[9], 7);

    /* 0x06 */
    alarm.failureOfCurrentSensorModule = bitRead(this->my_rxBuffer[10], 0);
    alarm.failureOfMainVoltageSensorModule = bitRead(this->my_rxBuffer[10], 1);
    alarm.failureOfShortCircuitProtection = bitRead(this->my_rxBuffer[10], 2);
    alarm.failureOfLowVoltageNoCharging = bitRead(this->my_rxBuffer[10], 3);

    return true;
}

bool PI_Serial::setDischargeMOS(bool sw) // 0xD9 0x80 First Byte 0x01=ON 0x00=OFF
{
    if (sw)
    {
        BMS_DEBUG_PRINTLN("Attempting to switch discharge MOSFETs on");
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    else
    {
        BMS_DEBUG_PRINTLN("Attempting to switch discharge MOSFETs off");
        this->sendCommand(COMMAND::DISCHRG_FET);
    }
    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
        return false;
    }

    return true;
}

bool PI_Serial::setChargeMOS(bool sw) // 0xDA 0x80 First Byte 0x01=ON 0x00=OFF
{
    if (sw == true)
    {
        BMS_DEBUG_PRINTLN("Attempting to switch charge MOSFETs on");
        // Set the first byte of the data payload to 1, indicating that we want to switch on the MOSFET
        this->my_txBuffer[4] = 0x01;
        this->sendCommand(COMMAND::CHRG_FET);
    }
    else
    {
        BMS_DEBUG_PRINTLN("Attempting to switch charge MOSFETs off");
        this->sendCommand(COMMAND::CHRG_FET);
    }

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify MOSFETs switched.\n");
        return false;
    }

    return true;
}

bool PI_Serial::setBmsReset() // 0x00 Reset the BMS
{
    this->sendCommand(COMMAND::BMS_RESET);

    if (!this->receiveBytes())
    {
        BMS_DEBUG_PRINT("<DALY-BMS DEBUG> Send failed, can't verify BMS was reset!\n");
        return false;
    }

    return true;
}

bool PI_Serial::setSOC(float val) // 0x21 last two byte is SOC
{
    if (val >= 0 && val <= 100)
    {
        BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to read the SOC");
        // try read with 0x61
        this->sendCommand(COMMAND::READ_SOC);
        if (!this->receiveBytes())
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> 0x61 read failed");
            // if 0x61 fails, write fake timestamp
            BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to set the SOC with fake RTC data");
            this->my_txBuffer[5] = 0x17; // year
            this->my_txBuffer[6] = 0x01; // month
            this->my_txBuffer[7] = 0x01; // day
            this->my_txBuffer[8] = 0x01; // hour
            this->my_txBuffer[9] = 0x01; // minute
        }
        else
        {
            BMS_DEBUG_PRINTLN("<DALY-BMS DEBUG> Attempting to set the SOC with RTC data from BMS");
            for (size_t i = 5; i <= 9; i++)
            {
                this->my_txBuffer[i] = this->my_rxBuffer[i];
            }
        }
        uint16_t value = (val * 10);
        this->my_txBuffer[10] = (value & 0xFF00) >> 8;
        this->my_txBuffer[11] = (value & 0x00FF);
        this->sendCommand(COMMAND::SET_SOC);

        if (!this->receiveBytes())
        {
            BMS_DEBUG_PRINT("<DALY-BMS DEBUG> No response from BMS! Can't verify SOC.\n");
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

int PI_Serial::getState() // Function to return the state of connection
{
    return get.connectionState;
}
*/
// start up save config callback
void PI_Serial::callback(std::function<void()> func)
{
    requestCallback = func;
}

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
const char *requestData(String command)
{
    String commandBuffer = "";

    SerialInverter.print(appendCRC(command));
    SerialInverter.print("\r");

    commandBuffer = SerialInverter.readStringUntil('\r');
#ifdef SERIALDEBUG
    Serial.print(F("Sending:\t"));
    Serial.print(command);
    Serial.print(F("\tCalc: "));
    Serial.print(getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)), HEX);
    Serial.print(F("\tRx: "));
    Serial.println(256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1], HEX);
    Serial.print(F("Recived:\t"));
    Serial.println(_commandBuffer.substring(0, _commandBuffer.length() - 2).c_str());
#endif
    commandBuffer.remove(_commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));        // remove the start character

    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[_commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
    {
        commandBuffer = "ERCRC";
    }
    return commandBuffer.c_str();
}

void PI_Serial::clearGet(void)
{
    /*
    // data from 0x90
    get.packVoltage = NAN; // pressure (0.1 V)
    get.packCurrent = NAN; // acquisition (0.1 V)
    get.packSOC = NAN;     // State Of Charge

    // data from 0x91
    get.maxCellmV = NAN; // maximum monomer voltage (mV)
    get.maxCellVNum = 0; // Maximum Unit Voltage cell No.
    get.minCellmV = NAN; // minimum monomer voltage (mV)
    get.minCellVNum = 0; // Minimum Unit Voltage cell No.
    get.cellDiff = NAN;  // difference betwen cells

    // data from 0x92
    get.tempAverage = 0; // Avergae Temperature
    */
    // data from 0x93
    //get.chargeDischargeStatus = "offline"; // charge/discharge status (0 stationary ,1 charge ,2 discharge)
    /*
    get.chargeFetState = NAN;       // charging MOS tube status
    get.disChargeFetState = NAN;    // discharge MOS tube state
    get.bmsHeartBeat = 0;           // BMS life(0~255 cycles)
    get.resCapacitymAh = 0;         // residual capacity mAH

    // data from 0x94
    get.numberOfCells = 0;                   // amount of cells
    get.numOfTempSensors = 0;                // amount of temp sensors
    get.chargeState = NAN;                   // charger status 0=disconnected 1=connected
    get.loadState = NAN;                     // Load Status 0=disconnected 1=connected
    memset(get.dIO, false, sizeof(get.dIO)); // No information about this
    get.bmsCycles = 0;                       // charge / discharge cycles

    // data from 0x95
    memset(get.cellVmV, 0, sizeof(get.cellVmV)); // Store Cell Voltages in mV

    // data from 0x96
    memset(get.cellTemperature, 0, sizeof(get.cellTemperature)); // array of cell Temperature sensors

    // data from 0x97
    memset(get.cellBalanceState, false, sizeof(get.cellBalanceState)); // bool array of cell balance states
    get.cellBalanceActive = NAN;                                       // bool is cell balance active
    */
}

String appendCRC(String data) // calculate and add the crc to the string
{
  crc.reset();
  crc.setPolynome(0x1021);
  crc.add((uint8_t *)data.c_str(), data.length());
  typedef union
  {
    struct
    {
      char cL;
      char cH;
    };
    uint16_t u;
  } cu_t;
  cu_t v;
  v.u = crc.getCRC();
  data.concat(v.cH);
  data.concat(v.cL);

  return data;
}

uint16_t getCRC(String data) // get a calculated crc from a string
{
  crc.reset();
  crc.setPolynome(0x1021);
  crc.add((uint8_t *)data.c_str(), data.length());
  return crc.getCRC(); // here comes the crc;
}