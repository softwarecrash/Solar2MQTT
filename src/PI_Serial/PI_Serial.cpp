#define isDEBUG



#include "PI_Serial.h"
SoftwareSerial myPort;
#include "CRC16.h"
#include "CRC.h"
CRC16 crc;
#include "Q/PIGS.h"
#include "Q/PIRI.h"
#include "Q/MOD.h"
#include "Q/QALL.h"



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

        PI_DEBUG_PRINT("Match protocol number: ");
        PI_DEBUG_PRINTLN(protocolType);
        return true;
    }
    else
    {
        PI_DEBUG_PRINT("protocol number: ");
        PI_DEBUG_PRINTLN(protocolType);
        return false;
    }
}

bool PI_Serial::update()
{
    // kommt später
    return true;
}

bool PI_Serial::getVariableData() // request the variable data
{
    String commandAnswer;
    switch (protocolType)
    {
    case PIXX:

        if(qAvaible.qpigs)PIXX_QPIGS();
        if(qAvaible.qall)PIXX_QALL();
        //PIXX_QPIGS2();
        //PIXX_QALL();
        if(qAvaible.qmod)PIXX_QMOD();
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
    case PIXX:
        if(qAvaible.qpiri)PIXX_QPIRI();
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
    PI_DEBUG_PRINT(F("Sending:\t"));
    PI_DEBUG_PRINT(command);
    PI_DEBUG_PRINT(F("\tCalc: "));
    PI_DEBUG_PRINT(getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)), HEX);
    PI_DEBUG_PRINT(F("\tRx: "));
    PI_DEBUG_PRINTLN(256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1], HEX);
    PI_DEBUG_PRINT(F("Recived:\t"));
    PI_DEBUG_PRINTLN(commandBuffer.substring(0, commandBuffer.length() - 2).c_str());
    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
    {
        PI_DEBUG_PRINTLN("ERCRC");
        return commandBuffer = "ERCRC";
    }
    commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));       // remove the start character
    PI_DEBUG_PRINT("Command Length: ");
    PI_DEBUG_PRINTLN(commandBuffer.length());
    return commandBuffer;
}
//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
unsigned int PI_Serial::autoDetect() // function for autodetect the inverter type
{
    if (protocolType == 100)
    {
        for (size_t i = 0; i < 3; i++) // try 3 times to detect the inverter
        {
            PI_DEBUG_PRINT("Try Autodetect Protocol");
            serialIntfBaud = 2400;
            // this->my_serialIntf->setTimeout(250);
            this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);

            String qpi = this->requestData("QPI");
            PI_DEBUG_PRINTLN("QPI:\t\t" + qpi + " (Length: " + qpi.length() + ")");
            String qpiri = this->requestData("QPIRI");
            PI_DEBUG_PRINTLN("QPIRI:\t\t" + qpiri + " (Length: " + qpiri.length() + ")");
            String qpigs = this->requestData("QPIGS");
            PI_DEBUG_PRINTLN("QPIGS:\t\t" + qpigs + " (Length: " + qpigs.length() + ")");

            if (
                (qpiri.length() == 94 && qpigs.length() == 90) || // typical length of PI30_HS_MS_MSX
                (qpiri.length() == 94 && qpigs.length() == 106)   // typical length of PI30_PI
            )
            {
                protocolType = PIXX;
                PI_DEBUG_PRINT("Match protocol number: ");
                PI_DEBUG_PRINTLN(protocolType);
            }
            this->my_serialIntf->end();

            if (protocolType != 100) // protocol found, break and report it
                break;
        }
    }
    return protocolType;
}

String PI_Serial::requestData(String command)
{
    String commandBuffer = "";
    this->my_serialIntf->print(appendCRC(command));
    this->my_serialIntf->print("\r");
    commandBuffer = this->my_serialIntf->readStringUntil('\r');

    PI_DEBUG_PRINTLN();
    PI_DEBUG_PRINT(F("Sending:\t"));
    PI_DEBUG_PRINT(command);
    PI_DEBUG_PRINT(F("\tCalc: "));
    PI_DEBUG_PRINT(getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)), HEX);
    PI_DEBUG_PRINT(F("\tRx: "));
    PI_DEBUG_PRINTLN(256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1], HEX);
    PI_DEBUG_PRINT(F("Recived:\t"));
    PI_DEBUG_PRINTLN(commandBuffer.substring(0, commandBuffer.length() - 2).c_str());

    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1])
    {
        PI_DEBUG_PRINTLN("ERCRC");
        return commandBuffer = "ERCRC";
    }
    commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
    commandBuffer.remove(0, strlen(startChar));       // remove the start character
    PI_DEBUG_PRINT("Command Length: ");
    PI_DEBUG_PRINTLN(commandBuffer.length());
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
    return -1; //befor it was return 0
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
    return -1; //befor it was return 0
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
    return NAN; //before it was return false
}

char *PI_Serial::getModeDesc(char mode) // get the char from QMOD and make readable things
{
    char *modeString;
    switch (mode)
    {
    default:
        modeString = (char *)("Undefined, Origin: " + mode);
        break;
    case 'P':
        modeString = (char *)"Power On";
        break;
    case 'S':
        modeString = (char *)"Standby";
        break;
    case 'Y':
        modeString = (char *)"Bypass";
        break;
    case 'L':
        modeString = (char *)"Line";
        break;
    case 'B':
        modeString = (char *)"Battery";
        break;
    case 'T':
        modeString = (char *)"Battery Test";
        break;
    case 'F':
        modeString = (char *)"Fault";
        break;
    case 'D':
        modeString = (char *)"Shutdown";
        break;
    case 'G':
        modeString = (char *)"Grid";
        break;
    case 'C':
        modeString = (char *)"Charge";
        break;
    }
    return modeString;
}