#include "Arduino.h"
#include "inverter.h"
#include "tickCounter.h"

#include "settings.h"

extern TickCounter _tickCounter;
extern Settings _settings;
extern byte inverterType; 
extern byte MPI;
extern byte PCM60x;
extern byte PIP;
extern int Led_Red;
extern int Led_Green;

String _setCommand;
String _commandBuffer;
String _otherBuffer ="";
String _lastRequestedCommand = "-"; //Set to not empty to force a timeout on startup
PollDelay _lastRequestedAt(_tickCounter);
String _nextCommandNeeded = "";
bool _allMessagesUpdated = false;
bool _otherMessagesUpdated = false;
PollDelay _lastReceivedAt(_tickCounter);


// PCM and PIP inverters use below
QpiMessage _qpiMessage = {0};
QpigsMessage _qpigsMessage = {0};
QmodMessage _qmodMessage = {0};
QpiwsMessage _qpiwsMessage = {0};
QflagMessage _qflagMessage = {0};
QidMessage _qidMessage = {0};
//new testings
QetMessage _qetMessage = {0};
QpiriMessage _qpiriMessage = {0};
QtMessage _qtMessage = {0};


QRaw _qRaw;
QAv _qAv;

//MPI Inverters use below
//P003GSMessage _P003GSMessage = {0};
//P003PSMessage _P003PSMessage = {0};
//P006FPADJMessage _P006FPADJMessage = {0};

//Found here: http://forums.aeva.asn.au/pip4048ms-inverter_topic4332_post53760.html#53760
#define INT16U unsigned int
#define INT8U byte

//short map function for float mapping
float mapf(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
  float result;
  result = (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
  return result;
} 

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
/*
bool onP003PS()
{
  //P003PS -- '81'^D07700139,00122,,,,,,0502,0973,0385,01860,0593,1040,0399,02032,031,1,1,1,1,2,1}⸮'
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
 
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
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  
  if (_commandBuffer.length() < 34)
    return false;
  int index = 5; //after the starting 'commands'
  _P006FPADJMessage.dir = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.watt = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.feedingGridDirectionR = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.calibrationWattR = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.feedingGridDirectionS = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.calibrationWattS = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.feedingGridDirectionT = (float)getNextFloat(_commandBuffer, index);
  _P006FPADJMessage.calibrationWattT = (float)getNextFloat(_commandBuffer, index);
  
  return true;
}

bool onP003GS()
{
  //P003GS -- '114'^D1103462,3468,0040,0035,0503,071,+00000,2369,2367,2350,5000,0000,0000,0000,2371,2365,2352,5000,,,,025,028,000,0b'
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  
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
*/

bool onPIGS() //QPIGS<cr>: Device general status parameters inquiry
{
#ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
#endif
  if(_commandBuffer[1]=='N') _qAv.QPIGS = false; //deactivate if NAK
  if (_commandBuffer.length() < 109)
  {
    return false;
  }
  else
  {
    _qRaw.QPIGS = _commandBuffer; //answer from a smv
    //(1)000.0 (2)00.0 (3)230.1 (4)50.0 (5)0000 (6)0000 (7)000 (8)336 (9)24.36 (10)000 (11)065 (12)0015 (13)0000 (14)000.0 (15)00.00 (16)00000 (17)00010000 (18)00 (19)00 (20)00000 (21)010

    int index = 1; //after the starting '('
    _qpigsMessage.rxTimeSec = _tickCounter.getSeconds();
    _qpigsMessage.gridV = getNextFloat(_commandBuffer, index);               //1
    _qpigsMessage.gridHz = getNextFloat(_commandBuffer, index);              //2
    _qpigsMessage.acOutV = getNextFloat(_commandBuffer, index);              //3
    _qpigsMessage.acOutHz = getNextFloat(_commandBuffer, index);             //4
    _qpigsMessage.acOutVa = (short)getNextLong(_commandBuffer, index);       //5
    _qpigsMessage.acOutW = (short)getNextLong(_commandBuffer, index);        //6
    _qpigsMessage.acOutPercent = (byte)getNextLong(_commandBuffer, index);   //7
    _qpigsMessage.busV = (short)getNextLong(_commandBuffer, index);          //8
    _qpigsMessage.battV = getNextFloat(_commandBuffer, index);               //9
    _qpigsMessage.battChargeA = (byte)getNextLong(_commandBuffer, index);    //10
    _qpigsMessage.battPercent = (byte)getNextLong(_commandBuffer, index);    //11
    _qpigsMessage.heatSinkDegC = getNextFloat(_commandBuffer, index);        //12
    _qpigsMessage.solarA = (byte)getNextLong(_commandBuffer, index);         //13
    _qpigsMessage.solarV = (byte)getNextLong(_commandBuffer, index);         //14
    _qpigsMessage.sccBattV = getNextFloat(_commandBuffer, index);            //15
    _qpigsMessage.battDischargeA = (byte)getNextLong(_commandBuffer, index); //16

    _qpigsMessage.addSbuPriorityVersion = getNextLong(_commandBuffer, index);             //17
    _qpigsMessage.isConfigChanged = getNextLong(_commandBuffer, index);                   //18
    _qpigsMessage.isSccFirmwareUpdated = getNextFloat(_commandBuffer, index);             //19
    _qpigsMessage.solarW = getNextFloat(_commandBuffer, index);                           //20
    _qpigsMessage.battVoltageToSteadyWhileCharging = getNextFloat(_commandBuffer, index); //21
    _qpigsMessage.chargingStatus = getNextLong(_commandBuffer, index);                    //22
    _qpigsMessage.reservedY = getNextLong(_commandBuffer, index);                         //23
    _qpigsMessage.reservedZ = getNextLong(_commandBuffer, index);                         //24
    _qpigsMessage.reservedAA = getNextLong(_commandBuffer, index);                        //25
    _qpigsMessage.reservedBB = getNextLong(_commandBuffer, index);                        //26

    //Beta
    //calculate the real SOC
    if (_qpigsMessage.sccBattV > _qpigsMessage.battV)
    {
      _qpigsMessage.cSOC = round(mapf(_qpigsMessage.battV, 22, 29, 0, 100));
    }
    else
    {
      _qpigsMessage.cSOC = round(mapf(_qpigsMessage.battV, 22, 27.6, 0, 100));
    }
    return true;
  }
}

bool onPIRI() //QPIRI<cr>: Device Rating Information inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if(_commandBuffer[1]=='N') _qAv.QPIRI = false; //deactivate if NAK
  if (_commandBuffer.length() < 45)
  {
    return false;
  }
  else
  {
    _qRaw.QPIRI = _commandBuffer;
    //(230.0 13.0 230.0 50.0 13.0 3000 3000 24.0 23.0 21.5 28.2 27.0 0 02 060 0 2 3 1 01 0 0 27.0 0 1
  int index = 1; //after the starting '('
  _qpiriMessage.gridRatingV =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.gridRatingA =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.acOutV =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.gridRatingHz =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.acOutA =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.gridRatingW =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.acOutRatingW =             getNextFloat(_commandBuffer, index);
  _qpiriMessage.battRatingV =             getNextFloat(_commandBuffer, index);
  
  return true;
  }
}

bool onMOD() //QMOD<cr>: Device Mode inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print(F("QMOD '"));
  Serial1.print(_commandBuffer);
  Serial1.print(F("'"));
  #endif
  if(_commandBuffer[1]=='N') _qAv.QMOD = false; //deactivate if NAK
  if (_commandBuffer.length() < 2)
  {
    return false;
  }
  else
  {
    _qRaw.QMOD = _commandBuffer;
    _qmodMessage.mode = _commandBuffer[1];
    switch (_commandBuffer[1])
    {
      default: _qmodMessage.operationMode = "Undefined, Origin: " +_commandBuffer[1];   break;
      case 'P': _qmodMessage.operationMode = "Power On";    break;
      case 'S': _qmodMessage.operationMode = "Standby";     break;
      case 'Y': _qmodMessage.operationMode = "Bypass";      break;
      case 'L': _qmodMessage.operationMode = "Line";        break;
      case 'B': _qmodMessage.operationMode = "Battery";     break;
      case 'T': _qmodMessage.operationMode = "Battery Test";break;
      case 'F': _qmodMessage.operationMode = "Fault";       break;
      case 'D': _qmodMessage.operationMode = "Shutdown";    break;
      case 'G': _qmodMessage.operationMode = "Grid";        break;
      case 'C': _qmodMessage.operationMode = "Charge";      break;
    }
    return true;
  }
}

bool onPIWS() //QPIWS<cr>: Device Warning Status inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if(_commandBuffer[1]=='N') _qAv.QPIWS = false; //deactivate if NAK
  if (_commandBuffer.length() < 32)
  {
    return false;
  }
  else
  {
  _qRaw.QPIWS = _commandBuffer;
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
}

bool onFLAG() //QFLAG<cr>: Device flag status inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if(_commandBuffer[1]=='N') _qAv.QFLAG = false; //deactivate if NAK
  if (_commandBuffer.length() < 10)
  {
    return false;
  }
  else
  {
  _qRaw.QFLAG = _commandBuffer;
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
}

bool onID() //QID<cr>: The device ID inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if(_commandBuffer[1]=='N') _qAv.QID = false; //deactivate if NAK
  if (_commandBuffer.length() < 15)
  {
    return false;
  }
  else
  {
  _qRaw.QID = _commandBuffer;
  //Discard the first '('
  _commandBuffer.substring(1).toCharArray(_qidMessage.id, sizeof(_qidMessage.id)-1); 
    
  return true;
  }
}

bool onPI() //QPI<cr>: Device Protocol ID Inquiry
{
  #ifdef SERIALDEBUG
  Serial1.print(F("QPI '"));
  Serial1.print(_commandBuffer);
  Serial1.print(F("'"));
  #endif
  if(_commandBuffer[1]=='N') _qAv.QPI = false; //deactivate if NAK
  if (_commandBuffer.length() < 5)
  {
    return false;
  }
  else
  {
    _qRaw.QPI = _commandBuffer;
    //Get number after '(PI'
    int index = 1;
    _qpiMessage.protocolId = (byte)getNextLong(_commandBuffer, index);
   
    return true;
  }
}

bool onQET() //QET<cr>: Inquiry total energy - BETA!!!!!
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if(_commandBuffer[1]=='N') _qAv.QET = false; //deactivate if NAK
  if (_commandBuffer.length() < 1 )
  {
  return false;
  }
  else
  {
  _qRaw.QET = _commandBuffer;
  //Get number after '('
  int index = 1;
  _qetMessage.energy = getNextLong(_commandBuffer, index);
   
  return true;
  }
}

bool onQT() //QT<cr>: Inquiry Device Date
{
#ifdef SERIALDEBUG
  Serial1.print("Processing data from: ");
  Serial1.println(_lastRequestedCommand);
#endif
  if (_commandBuffer[1]=='N') _qAv.QT = false;//deactivate if NAK
    if (_commandBuffer.length() < 1)
    {
      return false;
    }
    else
    {

      _qRaw.QT = _commandBuffer;
      //Get number after '('
      int index = 1;
      _qtMessage.deviceTime = getNextLong(_commandBuffer, index);

      return true;
    }
  
}

//Parse Other commands or so called RAWS
bool onOther()
{
  #ifdef SERIALDEBUG
  Serial1.print("Processing OTHER data from: ");
  Serial1.println(_lastRequestedCommand);
  #endif
  if (_commandBuffer.length() < 4)
    return false;  // If its below 4 the response is most likely false
  _otherMessagesUpdated = true;
  _otherBuffer = _commandBuffer;
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
      #ifdef SERIALDEBUG
      Serial1.print(F(" Calc: "));
      Serial1.print(calculatedCrc, HEX);
      Serial1.print(F(" Rx: "));
      Serial1.println(recievedCrc, HEX);
      #endif
    }
    #ifdef SERIALDEBUG
    Serial1.print(F("Command sent: "));
    Serial1.print(_lastRequestedCommand);
    Serial1.print(F(" Recieved data: "));
    Serial1.println(_commandBuffer);
    #endif
    
    //If CRC is okay, parse message and set next message to be requested
    if (calculatedCrc == recievedCrc)
    {
      /*
        //MPI
        digitalWrite(Led_Red, LOW);  //If we got a valid command show that on the led
        if (_lastRequestedCommand == "P003GS") 
        {
          onP003GS();
          _nextCommandNeeded = "P003PS";
        }
        else if (_lastRequestedCommand == "P003PS") 
        {
          onP003PS();
          _nextCommandNeeded = "P006FPADJ";
        }
  
        else if (_lastRequestedCommand == "P006FPADJ") 
        {
          onP006FPADJ();
          _nextCommandNeeded = "";
          _allMessagesUpdated = true;
        }
*/
      // Below for PCM
      if (_lastRequestedCommand == "QPIGS" ) //&& !inverterType
      {
          if(_qAv.QPIGS) onPIGS();
        _nextCommandNeeded = "QMOD";
      }
      else if (_lastRequestedCommand == "QMOD")
      {
        if(_qAv.QMOD) onMOD();
        _nextCommandNeeded = "QPIWS";
      }
      else if (_lastRequestedCommand == "QPIWS")
      {
        if(_qAv.QPIWS) onPIWS();
        _nextCommandNeeded = "QPIRI";
      }
      else if (_lastRequestedCommand == "QPIRI")
      {
        if(_qAv.QPIRI) onPIRI();
        _nextCommandNeeded = "QPI";
      }
      else if (_lastRequestedCommand == "QPI")
      {
        if(_qAv.QPI) onPI();
        _nextCommandNeeded = "QT";
      }
      else if (_lastRequestedCommand == "QT")
      {
        if(_qAv.QT) onQT();
        _nextCommandNeeded = "QID";
      }
      else if (_lastRequestedCommand == "QID")
      {
        if(_qAv.QID) onID();
        _nextCommandNeeded = "QFLAG";
      }
      else if (_lastRequestedCommand == "QFLAG")
      {
        if(_qAv.QFLAG) onFLAG();
        _nextCommandNeeded = "QET";
      }
      else if (_lastRequestedCommand == "QET")
      {
        if(_qAv.QET) onQET();
        _nextCommandNeeded = "";
        _allMessagesUpdated = true;
      }


    /*  
    //Below for PIP -- not actual
      else if (_lastRequestedCommand == "QMOD") 
      {
        onMOD();
        _nextCommandNeeded = "QPIWS";
      }
      else if (_lastRequestedCommand == "QPIWS") 
      {
        onPIWS();
        _nextCommandNeeded = "QFLAG";
      }
      else if (_lastRequestedCommand == "QFLAG") 
      {
        onFLAG();
        _nextCommandNeeded = "QID";
      }
      else if (_lastRequestedCommand == "QID") 
      {
        onID();
        _nextCommandNeeded = "QET";//org. none
        //_allMessagesUpdated = true;
      }

      else if (_lastRequestedCommand == "QET") 
      {
        onQET();
        _nextCommandNeeded = "QPIRI";//org none
        //_allMessagesUpdated = true;
      }
      else if (_lastRequestedCommand == "QPIRI") 
      {
        onPIRI();
        _nextCommandNeeded = "";
        _allMessagesUpdated = true;
      }
*/
      // ***********  ALL OTHER **********
      else {
        onOther();
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
    digitalWrite(Led_Red, HIGH); //If we timeout and didnt get any response we need to shut the red led
    _commandBuffer = "";
    _lastRequestedCommand = "";
    _nextCommandNeeded = "";
  }

  //Wait a bit after receiving the last command before requesting the next one
  // Dont send until _allMessagesUpdated is false
  if ((_lastRequestedCommand == "") && (_lastReceivedAt.compare(INVERTER_COMMAND_DELAY_MS) > 0) &&(!_allMessagesUpdated))//-< if this the problem that not running?
  {
    if (_nextCommandNeeded == "") {
      if (inverterType)  _nextCommandNeeded = "P003GS"; //IF MPI we start with that order
      else _nextCommandNeeded = "QPIGS";  //if PIP/PCM QPIGS
    }

    if (_setCommand.length()) {  // Raw command incomming. Need to process it as next command
      _nextCommandNeeded = _setCommand;
    }
    
    unsigned short crc = cal_crc_half((byte*)_nextCommandNeeded.c_str(), _nextCommandNeeded.length());
    
    _lastRequestedCommand = _nextCommandNeeded;
    _lastRequestedAt.reset();
    #ifdef SERIALDEBUG
    Serial1.print(F("Sent Command: "));
    #endif
    if (inverterType) { Serial.print("^");
    #ifdef SERIALDEBUG
    Serial1.print("^");
    #endif
    }  //If MPI then prechar is needed
    Serial.print(_nextCommandNeeded);
    #ifdef SERIALDEBUG
    Serial1.println(_nextCommandNeeded);
    #endif
    if (!inverterType) Serial.print((char)((crc >> 8) & 0xFF)); //ONLY CRC fo PCM/PIP
    if (!inverterType) Serial.print((char)((crc >> 0) & 0xFF)); //ONLY CRC fo PCM/PIP
    Serial.print("\r");
   
   if (_setCommand.length()) { _nextCommandNeeded = ""; _setCommand = ""; }  // If it was RAW command reset it.
  }
 
  while (Serial.available() > 0)
  {
    c = Serial.read();
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
