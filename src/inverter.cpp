#include "Arduino.h"


#include "EspSoftSerialRx.h" //Copied from: https://github.com/scottwday/EspSoftSerial
#include "main.h"
#include "inverter.h"
#include "tickCounter.h"
//#include "thingspeak.h"
#include "settings.h"

extern TickCounter _tickCounter;
extern EspSoftSerialRx SerialRx;
extern Settings _settings;
extern byte inverterType; 
extern byte MPI;
extern byte PCM60x;
extern byte PIP;

String _commandBuffer;
String _lastRequestedCommand = "-"; //Set to not empty to force a timeout on startup
PollDelay _lastRequestedAt(_tickCounter);
String _nextCommandNeeded = "";
bool _allMessagesUpdated = false;
PollDelay _lastReceivedAt(_tickCounter);


// PCM and PIP inverters use below
QpiMessage _qpiMessage = {0};
QpigsMessage _qpigsMessage = {0};
QmodMessage _qmodMessage = {0};
QpiwsMessage _qpiwsMessage = {0};
QflagMessage _qflagMessage = {0};
QidMessage _qidMessage = {0};


//MPI Inverters use below
P003GSMessage _P003GSMessage = {0};
P003PSMessage _P003PSMessage = {0};
P006FPADJMessage _P006FPADJMessage = {0};



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

bool onP003PS()
{
  //P003PS -- '81'^D07700139,00122,,,,,,0502,0973,0385,01860,0593,1040,0399,02032,031,1,1,1,1,2,1}⸮'
  Serial.print("P003PS -- '");
  Serial.print(_commandBuffer.length());
  Serial.print("'");
  Serial.print(_commandBuffer);
  Serial.println("'");
  
  if (_commandBuffer.length() < 81)
    return false;
  int index = 5; //after the starting 'commands'
  _P003PSMessage.rxTimeSec = _tickCounter.getSeconds();
  _P003PSMessage.solarWatt1 = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.solarWatt2 = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.batteryWatt = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.acin2_r = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.acin2_s = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.acin2_t = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.acin2_total = (float)getNextFloat(_commandBuffer, index)/10;

  _P003PSMessage.w_r = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.w_s = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.w_t = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.w_total = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.va_r = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.va_s = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.va_t = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.va_total = (float)getNextFloat(_commandBuffer, index)/10;
  _P003PSMessage.ac_output_procent = (short)getNextFloat(_commandBuffer, index);
 
       

  return true;

}

bool onP006FPADJ()
{
  //P006FPADJ -- '34'^D0301,0000,1,0099,1,0109,1,0112⸮7'
  Serial.print("P006FPADJ -- '");
  Serial.print(_commandBuffer.length());
  Serial.print("'");
  Serial.print(_commandBuffer);
  Serial.println("'");
  
  if (_commandBuffer.length() < 34)
    return false;
  int index = 5; //after the starting 'commands'
  return true;
}

bool onP003GS()
{
  //P003GS -- '114'^D1103462,3468,0040,0035,0503,071,+00000,2369,2367,2350,5000,0000,0000,0000,2371,2365,2352,5000,,,,025,028,000,0b'
  Serial.print("P003GS -- '");
  Serial.print(_commandBuffer.length());
  Serial.print("'");
  Serial.print(_commandBuffer);
  Serial.println("'");
  
  if (_commandBuffer.length() < 114)
    return false;
//P003GS -- '114'^D1103464,3454,0032,0026,0503,071,+00000,2395,2401,2374,5000,0000,0000,0000,2397,2399,2374,5000,,,,025,029,000,0⸮ '
  int index = 5; //after the starting 'commands'
  _P003GSMessage.rxTimeSec = _tickCounter.getSeconds();
  _P003GSMessage.solarInputV1 = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.solarInputV2 = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.solarInputA1 = (float)getNextFloat(_commandBuffer, index)/100;
  _P003GSMessage.solarInputA2 = (float)getNextFloat(_commandBuffer, index)/100;
  _P003GSMessage.battV = getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.battCapacity = getNextFloat(_commandBuffer, index);
  _P003GSMessage.battA = getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputVoltageR = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputVoltageS = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputVoltageT = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputFrequency = (float)getNextFloat(_commandBuffer, index)/100;
  _P003GSMessage.acInputCurrentR = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputCurrentS = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acInputCurrentT = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputVoltageR = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputVoltageS = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputVoltageT = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputFrequency = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputCurrentR = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputCurrentS = (float)getNextFloat(_commandBuffer, index)/10;
  _P003GSMessage.acOutputCurrentT = (float)getNextFloat(_commandBuffer, index)/10;
  return true;  
}



//Parse the response to QPIGS general status message, CRC has already been confirmed
bool onPIGS()
{
  Serial.print("QPIGS '");
  Serial.print(_commandBuffer);
  Serial.print("'");
  if (_commandBuffer.length() < 67)
    return false;

  int index = 1; //after the starting '('
  _qpigsMessage.rxTimeSec = _tickCounter.getSeconds();
  _qpigsMessage.solarV = (short)getNextFloat(_commandBuffer, index);
  _qpigsMessage.battV = getNextFloat(_commandBuffer, index);
  _qpigsMessage.battChargeA = getNextFloat(_commandBuffer, index);
  _qpigsMessage.solarA = getNextFloat(_commandBuffer, index);
  _qpigsMessage.solar2A = getNextFloat(_commandBuffer, index);
  _qpigsMessage.wattage = getNextFloat(_commandBuffer, index);
  
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
  if ((_commandBuffer.length() > 3) )
  {
    //&& (_commandBuffer[0] == '(')
    unsigned short calculatedCrc = cal_crc_half((byte*)_commandBuffer.c_str(), _commandBuffer.length() - 2);
    unsigned short recievedCrc = ((unsigned short)_commandBuffer[_commandBuffer.length()-2] << 8) | 
                                                  _commandBuffer[_commandBuffer.length()-1];
    if (!inverterType) {
      Serial.print(" Calc: ");
      Serial.print(calculatedCrc, HEX);
      Serial.print(" Rx: ");
      Serial.println(recievedCrc, HEX);
    }
    //If CRC is okay, parse message and set next message to be requested
    if (calculatedCrc == recievedCrc)
    {
//MPI
        if (_lastRequestedCommand == "P003GS") 
        {
          if (onP003GS()) _allMessagesUpdated = false;
          _nextCommandNeeded = "P003PS";
        }
        if (_lastRequestedCommand == "P003PS") 
        {
          if (onP003PS()) _allMessagesUpdated = false;
          _nextCommandNeeded = "P006FPADJ";
        }
  
        if (_lastRequestedCommand == "P006FPADJ") 
        {
          if (onP006FPADJ()) _allMessagesUpdated = true;
          _nextCommandNeeded = "";
        }

      // Below for PCM

      if (_lastRequestedCommand == "QPIGS" && !inverterType) 
      {
        if (onPIGS()) {
          _allMessagesUpdated = true;
        }
        _nextCommandNeeded = "";
      }

      
//Below for PIP
      
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
      if (inverterType)  _nextCommandNeeded = "P003GS"; //IF MPI we start with that order
      else _nextCommandNeeded = "QPIGS";  //if PIP 
  
    unsigned short crc = cal_crc_half((byte*)_nextCommandNeeded.c_str(), _nextCommandNeeded.length());
  
    _lastRequestedCommand = _nextCommandNeeded;
    _lastRequestedAt.reset();
    //Serial.print("Next command: ");
    //Serial.print(_nextCommandNeeded);
    if (inverterType) Serial1.print("^");  //If MPI then prechar is needed
    Serial1.print(_nextCommandNeeded);
    if (!inverterType) Serial1.print((char)((crc >> 8) & 0xFF)); //ONLY CRC fo PCM/PIP
    if (!inverterType) Serial1.print((char)((crc >> 0) & 0xFF)); //ONLY CRC fo PCM/PIP
    Serial1.print("\r");
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
