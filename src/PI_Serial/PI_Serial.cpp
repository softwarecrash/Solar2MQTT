#include "PI_Serial.h"
SoftwareSerial myPort;
#include "CRC16.h"
#include "CRC.h"
CRC16 crc;

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
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        BMS_DEBUG_PRINTLN("<PI SERIAL> ERROR: No serial peripheral specificed!");
        return false;
    }

    this->my_serialIntf->begin(2400, SWSERIAL_8N1, SERIAL_RX, SERIAL_TX, false);
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
   return 4;
}

bool PI_Serial::setProtocol(int protocolID)
{
    protocolID = PI30_MAX;
    return true;
}

bool PI_Serial::update()
{

    return true;
}



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

    this->my_serialIntf->print(appendCRC(command));
    this->my_serialIntf.print("\r");

    commandBuffer = this->my_serialIntf->readStringUntil('\r');
#ifdef SERIALDEBUG
    Serial.print(F("Sending:\t"));
    Serial.print(command);
    Serial.print(F("\tCalc: "));
    Serial.print(getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)), HEX);
    Serial.print(F("\tRx: "));
    Serial.println(256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1], HEX);
    Serial.print(F("Recived:\t"));
    Serial.println(commandBuffer.substring(0, commandBuffer.length() - 2).c_str());
#endif
    commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));        // remove the start character

    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
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

String PI_Serial::appendCRC(String data) // calculate and add the crc to the string
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

uint16_t PI_Serial::getCRC(String data) // get a calculated crc from a string
{
  crc.reset();
  crc.setPolynome(0x1021);
  crc.add((uint8_t *)data.c_str(), data.length());
  return crc.getCRC(); // here comes the crc;
}