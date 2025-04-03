// #define isDEBUG
#include "ArduinoJson.h"
#include "PI_Serial.h"
SoftwareSerial myPort;
#include "CRC16.h"
#include "CRC.h"
CRC16 crc;
// static
#include "QPI.h"
#include "QPIRI.h"
#include "QMN.h"
// variable
#include "Q1.h"
#include "QPIGS.h"
#include "QPIGS2.h"
#include "QMOD.h"
#include "QEX.h"
extern void writeLog(const char *format, ...);
//----------------------------------------------------------------------
//  Public Functions
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
        writeLog("No serial specificed!");
        return false;
    }
    autoDetect();
    this->my_serialIntf->setTimeout(500);
    this->my_serialIntf->enableRxGPIOPullUp(true);
    this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);
    return true;
}

bool PI_Serial::loop()
{
    if (millis() - previousTime > delayTime)
    {
        if (protocol != NoD)
        {
            if (sendCustomCommand())
            {
                requestStaticData = true;
                requestCounter = 0;
                previousTime = millis();
                return true;
            }
            switch (requestStaticData)
            {
            case true:
                switch (requestCounter)
                {
                case 0:
                    requestCounter = PIXX_QPIRI() ? (requestCounter + 1) : 0;
                    break;
                case 1:
                    requestCounter = PIXX_QMN() ? (requestCounter + 1) : 0;
                    break;
                case 2:
                    requestCounter = PIXX_QPI() ? (requestCounter + 1) : 0;
                    requestCounter = 0;
                    requestStaticData = false;
                    break;
                }
                break;
            case false:
                switch (requestCounter)
                {
                case 0:
                    requestCounter = PIXX_QPIGS() ? (requestCounter + 1) : 0;
                    break;
                case 1:
                    requestCounter = PIXX_QPIGS2() ? (requestCounter + 1) : 0;
                    break;
                case 2:
                    requestCounter = PIXX_QMOD() ? (requestCounter + 1) : 0;
                    break;
                case 3:
                    requestCounter = PIXX_Q1() ? (requestCounter + 1) : 0;
                    break;
                case 4:
                    requestCounter = PIXX_QEX() ? (requestCounter + 1) : 0;
                    break;
                case 5:
                    requestCallback();
                    requestCounter = 0;
                    break;
                }
                break;
            }
            connection = (connectionCounter < 10) ? true : false;
            previousTime = millis();
        }
        else
        {
            previousTime = millis();
            requestCallback();
            connection = false;
        }
    }
    return true;
}

void PI_Serial::callback(std::function<void()> func)
{
    requestCallback = func;
}

String PI_Serial::sendCommand(String command)
{
    if (command == "")
    {
        return command;
    }
    customCommandBuffer = command;
    return command;
}
//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
void PI_Serial::autoDetect() // function for autodetect the inverter type
{
    String protocolName = "not found";
    writeLog("----------------- Start Autodetect -----------------");
    for (size_t i = 0; i < 3; i++) // try 3 times to detect the inverter
    {
        writeLog("Try Autodetect Protocol");

        startChar = "(";
        serialIntfBaud = 2400;
        this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);
        String qpi = this->requestData("QPI");
        writeLog("QPI:\t\t%s (Length: %d)", qpi,qpi.length());
        if (qpi != "" && qpi.substring(0, 2) == "PI")
        {
            writeLog("<Autodetect> Match protocol: PI3X");
            delimiter = " ";
            protocol = PI30;
            break;
        }
        startChar = "^Dxxx";
        this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1, soft_rx, soft_tx, false);
        String P005PI = this->requestData("^P005PI");
        writeLog("^P005PI:\t\t%s (Length: %d)", P005PI, P005PI.length());
        if (P005PI != "" && P005PI == "18")
        {
            writeLog("<Autodetect> Match protocol: PI18");
            delimiter = ",";
            protocol = PI18;
            break;
        }
        this->my_serialIntf->end();
    }
    writeLog("----------------- End Autodetect -----------------");
}

bool PI_Serial::sendCustomCommand()
{
    if (customCommandBuffer == "")
        return false;
    get.raw.commandAnswer = requestData(customCommandBuffer);
    customCommandBuffer = "";
    return true;
}

String PI_Serial::requestData(String command)
{
    String commandBuffer = "";
    uint16_t crcCalc = 0;
    uint16_t crcRecive = 0;

    this->my_serialIntf->write(command.c_str());
    this->my_serialIntf->write(highByte(getCRC(command)));
    this->my_serialIntf->write(lowByte(getCRC(command)));
    this->my_serialIntf->write(0x0D);
    this->my_serialIntf->flush();

    // for testing
    this->my_serialIntf->enableTx(true);
    delay(20);
    commandBuffer = this->my_serialIntf->readStringUntil('\r');
    this->my_serialIntf->enableTx(false);

    if (getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) == 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1] &&
        getCRC(commandBuffer.substring(0, commandBuffer.length() - 2)) != 0 && 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1] != 0)
    {
        crcCalc = 256U * (uint8_t)commandBuffer[commandBuffer.length() - 2] + (uint8_t)commandBuffer[commandBuffer.length() - 1];
        crcRecive = getCRC(commandBuffer.substring(0, commandBuffer.length() - 2));
        commandBuffer.remove(commandBuffer.length() - 2); // remove the crc
        commandBuffer.remove(0, strlen(startChar));       // remove the start char ( for Pi30 and ^Dxxx for Pi18

        //requestOK++;
        connectionCounter = 0;
    }
    else if (getCHK(commandBuffer.substring(0, commandBuffer.length() - 1)) + 1 == commandBuffer[commandBuffer.length() - 1] &&
             getCHK(commandBuffer.substring(0, commandBuffer.length() - 1)) + 1 != 0 && commandBuffer[commandBuffer.length() - 1] != 0 &&
             command == "QALL" // crude fix for the qall chk thing
             )                 // CHK for QALL
    {
        crcCalc = getCHK(commandBuffer.substring(0, commandBuffer.length() - 1)) + 1;
        crcRecive = commandBuffer[commandBuffer.length() - 1];
        commandBuffer.remove(commandBuffer.length() - 1); // remove the crc
        commandBuffer.remove(0, strlen(startChar));       // remove the start char ( for Pi30 and ^Dxxx for Pi18

        //requestOK++;
        connectionCounter = 0;
    }
    else if (commandBuffer.indexOf("NAK", strlen(startChar)) > 0) // catch NAK without crc
    {
        commandBuffer = "NAK";
    }
    else if (commandBuffer == "") // catch empty answer, its similar to NAK
    {
        commandBuffer = "NAK";
    }
    else
    {
        writeLog("ERROR Send: >%s< Recive: >%s<",  command, commandBuffer);
        connectionCounter++;
        commandBuffer = "ERCRC";
    }
    writeLog("[C: %5S][CR: %4X][CC: %4X][L: %3u]", (const wchar_t *)command.c_str(), crcRecive, crcCalc, commandBuffer.length());
    return commandBuffer;
}

uint16_t cal_crc_half(uint8_t *pin, uint8_t len)
// https://forums.aeva.asn.au/viewtopic.php?t=4332&start=25
{
    uint16_t crc;
    uint8_t da;
    uint8_t *ptr;
    uint8_t bCRCHign;
    uint8_t bCRCLow;
    uint16_t crc_ta[16] =
        {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
            0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};
    ptr = pin;
    crc = 0;
    while (len-- != 0)
    {
        da = static_cast<uint8_t>((crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr >> 4)];
        da = static_cast<uint8_t>((crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr & 0x0f)];
        ptr++;
    }
    bCRCLow = crc;
    bCRCHign = static_cast<uint8_t>(crc >> 8);
    if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
    {
        bCRCLow++;
    }
    if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
    {
        bCRCHign++;
    }
    crc = static_cast<uint16_t>(bCRCHign) << 8;
    crc += bCRCLow;
    return crc;
}

uint16_t PI_Serial::getCRC(String data) // get a calculated crc from a string
{
    crc.reset();
    crc.setPolynome(0x1021);
    crc.add((uint8_t *)data.c_str(), data.length());
    // return crc.calc(); // here comes the crc;
    crc.calc();
    uint8_t CRCLow = lowByte(crc.calc());
    uint8_t CRCHigh = highByte(crc.calc());
    uint16_t CRCre;
    if (CRCLow == 0x28 || CRCLow == 0x0d || CRCLow == 0x0a)
    {
        CRCLow++;
    }
    if (CRCHigh == 0x28 || CRCHigh == 0x0d || CRCHigh == 0x0a)
    {
        CRCHigh++;
    }
    CRCre = ((uint16_t)CRCHigh) << 8;
    CRCre += CRCLow;
    return (CRCre);
}

byte PI_Serial::getCHK(String data) // get a calculatedt CHK
{
    byte chk = 0;
    for (unsigned int i = 0; i < data.length(); i++)
    {
        chk += data[i];
    }
    return chk;
}

char *PI_Serial::getModeDesc(char mode) // get the char from QMOD and make readable things
{
    char *modeString;
    switch (mode)
    {
    default:
        modeString = (char *)("");
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