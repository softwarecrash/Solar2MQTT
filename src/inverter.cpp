#include "Arduino.h"
#include <EspSoftSerialRx.h>

#include "main.h"
#include "inverter.h"
#include "tickCounter.h"
#include "thingspeak.h"
#include "settings.h"

extern TickCounter _tickCounter;
extern EspSoftSerialRx SerialRx;
extern Settings _settings;

String _commandBuffer;
String _lastRequestedCommand = "-"; //Set to not empty to force a timeout on startup
PollDelay _lastRequestedAt(_tickCounter);
String _nextCommandNeeded = "";
bool _allMessagesUpdated = false;
PollDelay _lastReceivedAt(_tickCounter);

QpiMessage _qpiMessage = {0};
QpigsMessage _qpigsMessage = {0};
QmodMessage _qmodMessage = {0};
QpiwsMessage _qpiwsMessage = {0};
QflagMessage _qflagMessage = {0};
QidMessage _qidMessage = {0};

//Found here: http://forums.aeva.asn.au/pip4048ms-inverter_topic4332_post53760.html#53760
#define INT16U unsigned int
#define INT8U byte
unsigned short cal_crc_half(byte* pin, byte len)
{
  unsigned short crc;
  byte da;
  byte *ptr;
  byte bCRCHign;
  byte bCRCLow;

  const unsigned short crc_ta[16]=
  { 
      0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
      0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef
  };
  
  ptr=pin;
  crc=0;
  while(len--!=0) 
  {
    da=((byte)(crc>>8))>>4; 
    crc<<=4;
    crc^=crc_ta[da^(*ptr>>4)]; 
    da=((byte)(crc>>8))>>4; 
    crc<<=4;
    crc^=crc_ta[da^(*ptr&0x0f)]; 
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign= (byte)(crc>>8);
  if(bCRCLow==0x28||bCRCLow==0x0d||bCRCLow==0x0a)
  {
    bCRCLow++;
  }
  if(bCRCHign==0x28||bCRCHign==0x0d||bCRCHign==0x0a)
  {
    bCRCHign++;
  }
  crc = ((unsigned short)bCRCHign)<<8;
  crc += bCRCLow;
  return(crc);
}

//Parses out the next number in the command string, starting at index
//updates index as it goes
float getNextFloat(String& command, int& index)
{
  String term = "";
  while (index < command.length())
  {
    char c = command[index];
    ++index;

    if ((c == '.') || ((c >= '0') && (c <= '9')))
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

//Parses out the next number in the command string, starting at index
//updates index as it goes
long getNextLong(String& command, int& index)
{
  String term = "";
  while (index < command.length())
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

//Gets if the next character is '1'
bool getNextBit(String& command, int& index)
{
  String term = "";
  if (index < command.length())
  {
    char c = command[index];
    ++index;
    return c == '1';
  }
  return false;
}

//Parse the response to QPIGS general status message, CRC has already been confirmed
bool onPIGS()
{
  Serial.print("QPIGS '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 109)
    return false;

  int index = 1; //after the starting '('
  _qpigsMessage.rxTimeSec = _tickCounter.getSeconds();
  _qpigsMessage.gridV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.gridHz = getNextFloat(_commandBuffer, index);
  _qpigsMessage.acOutV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.acOutHz = getNextFloat(_commandBuffer, index);
  _qpigsMessage.acOutVa = (short)getNextLong(_commandBuffer, index);
  _qpigsMessage.acOutW = (short)getNextLong(_commandBuffer, index);
  _qpigsMessage.acOutPercent = (byte)getNextLong(_commandBuffer, index);
  _qpigsMessage.busV = (short)getNextLong(_commandBuffer, index);
  _qpigsMessage.battV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.battChargeA = getNextFloat(_commandBuffer, index);
  _qpigsMessage.battPercent = getNextFloat(_commandBuffer, index);
  _qpigsMessage.heatSinkDegC = getNextFloat(_commandBuffer, index);
  _qpigsMessage.solarA = getNextFloat(_commandBuffer, index);
  _qpigsMessage.solarV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.sccBattV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.battDischargeA = getNextFloat(_commandBuffer, index);
  _qpigsMessage.addSbuPriorityVersion = getNextBit(_commandBuffer, index);
  _qpigsMessage.isConfigChanged = getNextBit(_commandBuffer, index);
  _qpigsMessage.isSccFirmwareUpdated = getNextBit(_commandBuffer, index);
  _qpigsMessage.isLoadOn = getNextBit(_commandBuffer, index);             
  _qpigsMessage.battVoltageToSteadyWhileCharging = getNextBit(_commandBuffer, index);
  _qpigsMessage.chargingStatus = (byte)getNextLong(_commandBuffer, index);
  _qpigsMessage.reservedY = (byte)getNextLong(_commandBuffer, index);
  _qpigsMessage.reservedZ = (byte)getNextLong(_commandBuffer, index);
  _qpigsMessage.reservedAA = getNextLong(_commandBuffer, index);
  _qpigsMessage.reservedBB = (short)getNextLong(_commandBuffer, index);

  return true;
}

//Parse the response to QMOD general status message, CRC has already been confirmed
bool onMOD()
{
  Serial.print("QMOD '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 2)
    return false;

  _qmodMessage.mode = _commandBuffer[1];

  return true;
}

//Parse the response to QMOD general status message, CRC has already been confirmed
bool onPIWS()
{
  Serial.print("QPIWS '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 32)
    return false;

  int index = 1; //after the starting '('
  _qpiwsMessage.reserved0 = getNextBit(_commandBuffer, index);
  _qpiwsMessage.inverterFault = getNextBit(_commandBuffer, index);
  _qpiwsMessage.busOver = getNextBit(_commandBuffer, index);
  _qpiwsMessage.busUnder = getNextBit(_commandBuffer, index);
  _qpiwsMessage.busSoftFail = getNextBit(_commandBuffer, index);
  _qpiwsMessage.lineFail = getNextBit(_commandBuffer, index);
  _qpiwsMessage.opvShort = getNextBit(_commandBuffer, index);
  _qpiwsMessage.overTemperature = getNextBit(_commandBuffer, index);
  _qpiwsMessage.fanLocked = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batteryVoltageHigh = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batteryLowAlarm = getNextBit(_commandBuffer, index);
  _qpiwsMessage.reserved13 = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batteryUnderShutdown = getNextBit(_commandBuffer, index);
  _qpiwsMessage.reserved15 = getNextBit(_commandBuffer, index);
  _qpiwsMessage.overload = getNextBit(_commandBuffer, index);
  _qpiwsMessage.eepromFault = getNextBit(_commandBuffer, index);
  _qpiwsMessage.inverterOverCurrent = getNextBit(_commandBuffer, index);
  _qpiwsMessage.inverterSoftFail = getNextBit(_commandBuffer, index);
  _qpiwsMessage.selfTestFail = getNextBit(_commandBuffer, index);
  _qpiwsMessage.opDcVoltageOver = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batOpen = getNextBit(_commandBuffer, index);
  _qpiwsMessage.currentSensorFail = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batteryShort = getNextBit(_commandBuffer, index);
  _qpiwsMessage.powerLimit = getNextBit(_commandBuffer, index);
  _qpiwsMessage.pvVoltageHigh = getNextBit(_commandBuffer, index);
  _qpiwsMessage.mpptOverloadFault = getNextBit(_commandBuffer, index);
  _qpiwsMessage.mpptOverloadWarning = getNextBit(_commandBuffer, index);
  _qpiwsMessage.batteryTooLowToCharge = getNextBit(_commandBuffer, index);
  _qpiwsMessage.reserved30 = getNextBit(_commandBuffer, index);
  _qpiwsMessage.reserved31 = getNextBit(_commandBuffer, index);

  return true;
}


//Parse the response to QFLAG flags message, CRC has already been confirmed
bool onFLAG()
{
  Serial.print("QFLAG '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 10)
    return false;

  int index = 1; //after the starting '('
  _qflagMessage.disableBuzzer = getNextBit(_commandBuffer, index);
  _qflagMessage.enableOverloadBypass = getNextBit(_commandBuffer, index);
  _qflagMessage.enablePowerSaving = getNextBit(_commandBuffer, index);
  _qflagMessage.enableLcdEscape = getNextBit(_commandBuffer, index);
  _qflagMessage.enableOverloadRestart = getNextBit(_commandBuffer, index);
  _qflagMessage.enableOvertempRestart = getNextBit(_commandBuffer, index);
  _qflagMessage.enableBacklight = getNextBit(_commandBuffer, index);
  _qflagMessage.enablePrimarySourceInterruptedAlarm = getNextBit(_commandBuffer, index);
  _qflagMessage.enableFaultCodeRecording = getNextBit(_commandBuffer, index);
  
  return true;
}

//Parse the response to QID device id message, CRC has already been confirmed
bool onID()
{
  Serial.print("QID '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 15)
    return false;

  //Discard the first '('
  _commandBuffer.substring(1).toCharArray(_qidMessage.id, sizeof(_qidMessage.id)-1); 
    
  return true;
}

//Parse the response to QPI protocol info message, CRC has already been confirmed
bool onPI()
{
  Serial.print("QPI '");
  Serial.print(_commandBuffer);
  Serial.print("'");

  if (_commandBuffer.length() < 5)
    return false;

  //Get number after '(PI'
  int index = 3;
  _qpiMessage.protocolId = (byte)getNextLong(_commandBuffer, index);
   
  return true;
}

//Called once a line has been received from the inverter (on CR)
void onInverterCommand()
{
  if ((_commandBuffer.length() > 3) && (_commandBuffer[0] == '('))
  {
    unsigned short calculatedCrc = cal_crc_half((byte*)_commandBuffer.c_str(), _commandBuffer.length() - 2);
    unsigned short recievedCrc = ((unsigned short)_commandBuffer[_commandBuffer.length()-2] << 8) | 
                                                  _commandBuffer[_commandBuffer.length()-1];

    Serial.print(" Calc: ");
    Serial.print(calculatedCrc, HEX);
    Serial.print(" Rx: ");
    Serial.println(recievedCrc, HEX);

    //If CRC is okay, parse message and set next message to be requested
    if (calculatedCrc == recievedCrc)
    {
      if (_lastRequestedCommand == "QPI") 
      {
        onPI();
        _nextCommandNeeded = "QPIGS";
      }
      if (_lastRequestedCommand == "QPIGS") 
      {
        onPIGS();
        _nextCommandNeeded = "QMOD";
      }
      if (_lastRequestedCommand == "QMOD") 
      {
        onMOD();
        _nextCommandNeeded = "QPIWS";
      }
      if (_lastRequestedCommand == "QPIWS") 
      {
        onPIWS();
        _nextCommandNeeded = "QFLAG";
      }
      if (_lastRequestedCommand == "QFLAG") 
      {
        onFLAG();
        _nextCommandNeeded = "QID";
      }
      if (_lastRequestedCommand == "QID") 
      {
        onID();
        _nextCommandNeeded = "";
        _allMessagesUpdated = true;
      }
    }
  }

  _lastReceivedAt.reset();
  _lastRequestedCommand = "";
}


//Parses incoming characters from the serial port
void serviceInverter()
{
  byte c;

  //Check time since last requested command
  if (_lastRequestedAt.compare(INVERTER_COMMAND_TIMEOUT_MS) > 0)
  {
    _commandBuffer = "";
    _lastRequestedCommand = "";
    _nextCommandNeeded = "";
  }

  //Wait a bit after receiving the last command before requesting the next one
  // Dont send until _allMessagesUpdated is false
  if ((_lastRequestedCommand == "") && (_lastReceivedAt.compare(INVERTER_COMMAND_DELAY_MS) > 0) && (!_allMessagesUpdated))
  {
    if (_nextCommandNeeded == "")
      _nextCommandNeeded = "QPI";
  
    unsigned short crc = cal_crc_half((byte*)_nextCommandNeeded.c_str(), _nextCommandNeeded.length());
  
    _lastRequestedCommand = _nextCommandNeeded;
    _lastRequestedAt.reset();

    Serial.println(_nextCommandNeeded);
  
    Serial1.print(_nextCommandNeeded);
    Serial1.print((char)((crc >> 8) & 0xFF));
    Serial1.print((char)((crc >> 0) & 0xFF));
    Serial1.print("\r\n");
  }
    
  while (SerialRx.read(c))
  {
    //Only accept incoming characters if we've requested something
    if (_lastRequestedCommand != "")
    {
      if ((c != 0) && (c != '\r') && (c != '\n'))
      {
        if (_commandBuffer.length() < 255)
          _commandBuffer += (char)c;
      }
      else if ((c == '\r') || (c == '\n'))
      {
        onInverterCommand();
        _commandBuffer = "";
      }
    }
  }   
}


