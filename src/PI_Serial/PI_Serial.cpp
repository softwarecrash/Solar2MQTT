#include "PI_Serial.h"
SoftwareSerial myPort;
#include "CRC16.h"
#include "CRC.h"
CRC16 crc;
#include "devices/PI30_HS_MS_MSX.h"
#include "devices/PI30_PIP.h"

#define SERIALDEBUG

const char *startChar = "("; // move later to changeable
//----------------------------------------------------------------------
// Public Functions
//----------------------------------------------------------------------

PI_Serial::PI_Serial(int rx, int tx)
{
    soft_rx = rx;
    soft_tx = tx;
    this->my_serialIntf = &myPort;
}

bool PI_Serial::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        // BMS_DEBUG_PRINTLN("<PI SERIAL> ERROR: No serial peripheral specificed!");
        return false;
    }

    autoDetect();
    //serialIntfBaud = 2400;
     


    this->my_serialIntf->setTimeout(250);
    this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);
    clearGet();
    return true;
}



bool PI_Serial::setProtocol(int protocolID)
{
    if (protocolID >= 0 && protocolID <= 20)
    {
        protocolType = protocolID;

        Serial.print("Match protocol number: ");
        Serial.println(protocolType);
        return true;
    }
    else
    {
        Serial.print("protocol number: ");
        Serial.println(protocolType);
        return false;
    }
}

bool PI_Serial::update()
{
    // kommt später
    return true;
}

bool PI_Serial::getVariableData() //request the variable data
{
    String commandAnswer;
    switch (protocolType)
    {
    case PI30_HS_MS_MSX:
        PI30_HS_MS_MSX_QPIGS();
        PI30_HS_MS_MSX_QMOD();
        break;
    case PI30_PIP://example
        PI30_PIP_QPIGS();//example
        PI30_PIP_QMOD();//example
        break;

    default:
        break;
    }
    return true;
}

bool PI_Serial::getStaticeData() // request static data
{
    String commandAnswer;
    switch (protocolType)
    {
    case PI30_HS_MS_MSX:
        PI30_HS_MS_MSX_QPIRI();
        break;
    case PI30_PIP://example
        PI30_PIP_QPIRI();//example
        break;

    default:
        break;
    }
    return true;
}

// start up save config callback
void PI_Serial::callback(std::function<void()> func)
{
    requestCallback = func;
}

String PI_Serial::sendCommand(String command)
{
    String commandBuffer = "";
    this->my_serialIntf->print(appendCRC(command));
    this->my_serialIntf->print("\r");
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
    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
    {
#ifdef SERIALDEBUG
        Serial.println("ERCRC");
#endif
        return commandBuffer = "ERCRC";
    }
    commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));       // remove the start character
#ifdef SERIALDEBUG
    Serial.print("Command Length: ");
    Serial.println(commandBuffer.length());
#endif
    return commandBuffer;
}
//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
unsigned int PI_Serial::autoDetect() // function for autodetect the inverter type
{
if(protocolType == 100)
{
Serial.print("Try Autodetect Protocol");
    serialIntfBaud = 2400;
    this->my_serialIntf->setTimeout(250);
    this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);

    if(this->requestData("QPIRI").length() == 94 && this->requestData("QPIGS").length() == 106) //typical length of PI30_HS_MS_MSX
    {
    protocolType = PI30_HS_MS_MSX;
    Serial.print("Match protocol number: ");
    Serial.println(protocolType);
    } else 
    if (this->requestData("QPIRI").length() == 94 && this->requestData("QPIGS").length() == 106)
    {
      protocolType = PI30_HS_MS_MSX;
    }
    
    //not ready, yust a first test if the autodetect working
    this->my_serialIntf->end();
}
    return protocolType;
}

String PI_Serial::requestData(String command)
{
    String commandBuffer = "";
    this->my_serialIntf->print(appendCRC(command));
    this->my_serialIntf->print("\r");
    commandBuffer = this->my_serialIntf->readStringUntil('\r');
#ifdef SERIALDEBUG
    Serial.println();
    Serial.print(F("Sending:\t"));
    Serial.print(command);
    Serial.print(F("\tCalc: "));
    Serial.print(getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)), HEX);
    Serial.print(F("\tRx: "));
    Serial.println(256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1], HEX);
    Serial.print(F("Recived:\t"));
    Serial.println(commandBuffer.substring(0, commandBuffer.length() - 2).c_str());
#endif
    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
    {
#ifdef SERIALDEBUG
        Serial.println("ERCRC");
#endif
        return commandBuffer = "ERCRC";
    }
    commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));       // remove the start character
#ifdef SERIALDEBUG
    Serial.print("Command Length: ");
    Serial.println(commandBuffer.length());
#endif
    return commandBuffer;
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
    // get.chargeDischargeStatus = "offline"; // charge/discharge status (0 stationary ,1 charge ,2 discharge)
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

float PI_Serial::getNextFloat(String &command, int &index) // Parses out the next long number
{
    String term = "";
    while (index < (int)command.length())
    {
        char c = command[index];
        ++index;

        if ((c == '.') || (c == '+') || (c == '-') || ((c >= '0') && (c <= '9')))
        {
            term += c;
        }
        else
        {
            return term.toFloat();
        }
    }
    return 0;
}

long PI_Serial::getNextLong(String &command, int &index) // Parses out the next number in the command string, starting at index
{
    String term = "";
    while (index < (int)command.length())
    {
        char c = command[index];
        ++index;

        if ((c == '.') || ((c >= '0') && (c <= '9')))
        {
            term += c;
        }
        else
        {
            return term.toInt();
        }
    }
    return 0;
}

bool PI_Serial::getNextBit(String &command, int &index) // Gets if the next character is '1'
{
    String term = "";
    if (index < (int)command.length())
    {
        char c = command[index];
        ++index;
        return c == '1';
    }
    return false;
}

String PI_Serial::getModeDesc(char mode) // get the char from QMOD and make readable things
{
    String modeString;
    switch (mode)
    {
    default:
        modeString = "Undefined, Origin: " + mode;
        break;
    case 'P':
        modeString = "Power On";
        break;
    case 'S':
        modeString = "Standby";
        break;
    case 'Y':
        modeString = "Bypass";
        break;
    case 'L':
        modeString = "Line";
        break;
    case 'B':
        modeString = "Battery";
        break;
    case 'T':
        modeString = "Battery Test";
        break;
    case 'F':
        modeString = "Fault";
        break;
    case 'D':
        modeString = "Shutdown";
        break;
    case 'G':
        modeString = "Grid";
        break;
    case 'C':
        modeString = "Charge";
        break;
    }
    return modeString;
}