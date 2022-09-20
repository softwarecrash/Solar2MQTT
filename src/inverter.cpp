#include "Arduino.h"
#include "inverter.h"

String _commandBuffer;

QpiMessage _qpiMessage = {0};
QpigsMessage _qpigsMessage = {0};
QmodMessage _qmodMessage = {0};
QpiwsMessage _qpiwsMessage = {0};
QflagMessage _qflagMessage = {0};
QidMessage _qidMessage = {0};
QchgcrMessage _qchgcrMessage ={0};


QpiriMessage _qpiriMessage = {0};


QRaw _qRaw;
//QAv _qAv;

//Found here: http://forums.aeva.asn.au/pip4048ms-inverter_topic4332_post53760.html#53760
#define INT16U unsigned int
#define INT8U byte

#define SERIALDEBUG

//short map function for float mapping
float mapf(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
  float result;
  result = (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
  return result;
}

unsigned short cal_crc_half(byte *pin, byte len)
{
  unsigned short crc;
  byte da;
  byte *ptr;
  byte bCRCHign;
  byte bCRCLow;

  const unsigned short crc_ta[16] =
      {
          0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
          0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};

  ptr = pin;
  crc = 0;
  while (len-- != 0)
  {
    da = ((byte)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr >> 4)];
    da = ((byte)(crc >> 8)) >> 4;
    crc <<= 4;
    crc ^= crc_ta[da ^ (*ptr & 0x0f)];
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign = (byte)(crc >> 8);
  if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
  {
    bCRCLow++;
  }
  if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
  {
    bCRCHign++;
  }
  crc = ((unsigned short)bCRCHign) << 8;
  crc += bCRCLow;
  return (crc);
}

//Parses out the next number in the command string, starting at index
//updates index as it goes
float getNextFloat(String &command, int &index)
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

//Parses out the next number in the command string, starting at index
//updates index as it goes
long getNextLong(String &command, int &index)
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

//Gets if the next character is '1'
bool getNextBit(String &command, int &index)
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

bool onPIGS() //QPIGS<cr>: Device general status parameters inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIGS = false; //deactivate if NAK
  if (_commandBuffer.length() < 60)
  {
    return false;
  }
  else
  {
    _qRaw.QPIGS = _commandBuffer;
    int index = 1; //after the starting '('
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
      _qpigsMessage.cSOC = constrain(round(mapf(_qpigsMessage.battV, _qpiriMessage.battreChargeV, _qpiriMessage.battBulkV, 0, 100)),0,100);
    }
    else
    {
      _qpigsMessage.cSOC = constrain(round(mapf(_qpigsMessage.battV, _qpiriMessage.battreChargeV, _qpiriMessage.battFloatV, 0, 100)),0,100);
    }
    return true;
  }
}

bool onPIRI() //QPIRI<cr>: Device Rating Information inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIRI = false; //deactivate if NAK
  if (_commandBuffer.length() < 45)
  {
    return false;
  }
  else
  {
    _qRaw.QPIRI = _commandBuffer;
    int index = 1; //after the starting '('
    _qpiriMessage.gridRatingV = getNextFloat(_commandBuffer, index); //BBB.B
    _qpiriMessage.gridRatingA = getNextFloat(_commandBuffer, index); //CC.C
    _qpiriMessage.acOutRatingV = getNextFloat(_commandBuffer, index); //DDD.D
    _qpiriMessage.acOutRatingHz = getNextFloat(_commandBuffer, index); //EE.E
    _qpiriMessage.acOutRatingA = getNextFloat(_commandBuffer, index); //FF.F
    _qpiriMessage.acOutRatungVA = getNextFloat(_commandBuffer, index); //HHHH
    _qpiriMessage.acOutRatingW = getNextFloat(_commandBuffer, index); //IIII
    _qpiriMessage.battRatingV = getNextFloat(_commandBuffer, index); //JJ.J

    _qpiriMessage.battreChargeV = getNextFloat(_commandBuffer, index); //KK.K
    _qpiriMessage.battUnderV = getNextFloat(_commandBuffer, index); //1JJ.J
    _qpiriMessage.battBulkV = getNextFloat(_commandBuffer, index); //1KK.K
    _qpiriMessage.battFloatV = getNextFloat(_commandBuffer, index); //LL.L


switch ((byte)getNextLong(_commandBuffer, index))//O
    {
    case 0: _qpiriMessage.battType = "AGM"; break;
    case 1: _qpiriMessage.battType = "Flooded"; break;
    case 2: _qpiriMessage.battType = "User"; break;
    }

    _qpiriMessage.battMaxAcChrgA = (byte)getNextLong(_commandBuffer, index); //PP
    _qpiriMessage.battMaxChrgA = (byte)getNextLong(_commandBuffer, index); //QO

    return true;
  }
}

bool onMOD() //QMOD<cr>: Device Mode inquiry
{
  // if(_commandBuffer[1]=='N') _qAv.QMOD = false; //deactivate if NAK
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
    default:
      _qmodMessage.operationMode = "Undefined, Origin: " + _commandBuffer[1];
      break;
    case 'P':
      _qmodMessage.operationMode = "Power On";
      break;
    case 'S':
      _qmodMessage.operationMode = "Standby";
      break;
    case 'Y':
      _qmodMessage.operationMode = "Bypass";
      break;
    case 'L':
      _qmodMessage.operationMode = "Line";
      break;
    case 'B':
      _qmodMessage.operationMode = "Battery";
      break;
    case 'T':
      _qmodMessage.operationMode = "Battery Test";
      break;
    case 'F':
      _qmodMessage.operationMode = "Fault";
      break;
    case 'D':
      _qmodMessage.operationMode = "Shutdown";
      break;
    case 'G':
      _qmodMessage.operationMode = "Grid";
      break;
    case 'C':
      _qmodMessage.operationMode = "Charge";
      break;
    }
    return true;
  }
}

bool onMCHGCR() //QMOD<cr>: Device Mode inquiry
{
  // if(_commandBuffer[1]=='N') _qAv.QMOD = false; //deactivate if NAK
  if (_commandBuffer.length() < 2)
  {
    return false;
  }
  else
  {
    int index = 1; //after the starting '('
    _qRaw.QMCHGCR = _commandBuffer;
    
    for (size_t i = 0; i <sizeof(_qchgcrMessage.chargeModes); i++)
    {
      _qchgcrMessage.chargeModes[i] = (byte)getNextLong(_commandBuffer, index);
    }

    return true;
  }
}

bool onMUCHGCR() //QMOD<cr>: Device Mode inquiry
{
  // if(_commandBuffer[1]=='N') _qAv.QMOD = false; //deactivate if NAK
  if (_commandBuffer.length() < 2)
  {
    return false;
  }
  else
  {
    int index = 1; //after the starting '('
    _qRaw.QMUCHGCR = _commandBuffer;
       for (size_t i = 0; i <sizeof(_qchgcrMessage.uChargeModes); i++)
    {
      _qchgcrMessage.uChargeModes[i] = (byte)getNextLong(_commandBuffer, index);
    }
    
    return true;
  }
}

bool onPIWS() //QPIWS<cr>: Device Warning Status inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPIWS = false; //deactivate if NAK
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
  //if(_commandBuffer[1]=='N') _qAv.QFLAG = false; //deactivate if NAK
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
  //if(_commandBuffer[1]=='N') _qAv.QID = false; //deactivate if NAK
  if (_commandBuffer.length() < 15)
  {
    return false;
  }
  else
  {
    _qRaw.QID = _commandBuffer;
    //Discard the first '('
    _commandBuffer.substring(1).toCharArray(_qidMessage.id, sizeof(_qidMessage.id) - 1);

    return true;
  }
}

bool onPI() //QPI<cr>: Device Protocol ID Inquiry
{
  //if(_commandBuffer[1]=='N') _qAv.QPI = false; //deactivate if NAK
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

bool sendCommand(String command)
{
  _commandBuffer = "";

  unsigned short crc = cal_crc_half((byte *)command.c_str(), command.length());

#ifdef SERIALDEBUG
  Serial1.print(F("Sent Command: "));
  Serial1.println(command);

#endif
  Serial.print(command);
  Serial.print((char)((crc >> 8) & 0xFF)); //ONLY CRC fo PCM/PIP
  Serial.print((char)((crc >> 0) & 0xFF)); //ONLY CRC fo PCM/PIP
  Serial.print("\r");


  _commandBuffer = Serial.readStringUntil('\n');

  unsigned short calculatedCrc = cal_crc_half((byte *)_commandBuffer.c_str(), _commandBuffer.length() - 2);
  unsigned short recievedCrc = ((unsigned short)_commandBuffer[_commandBuffer.length() - 2] << 8);

  //remove the CRC from recived command
  if (_commandBuffer[_commandBuffer.length() - 2] < 6)
  {
    _commandBuffer.remove(_commandBuffer.length() - 2);
  }
  else
  {
    _commandBuffer.remove(_commandBuffer.length() - 3);
  }

//for bugfix add a space at the end of commandstring
_commandBuffer.concat(" ");

#ifdef SERIALDEBUG
      Serial1.print(F("   Calc: "));
      Serial1.print(calculatedCrc, HEX);
      Serial1.print(F("   Rx: "));
      Serial1.println(recievedCrc, HEX);
      Serial1.print(F("   Recived: "));
      Serial1.println(_commandBuffer);
#endif

  if (calculatedCrc == recievedCrc)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void requestInverter(qCommand com)
{
  switch (com)
  {
  //case qCommand::QPI: if(sendCommand("QPI")) onPI(); break; //not needet, some inverters dont understand this
  case qCommand::QID: break;
  case qCommand::QVFW: break;
  case qCommand::QVFW2: break;
  case qCommand::QPIRI: if(sendCommand("QPIRI")) onPIRI(); break; //vervollständigen!
  case qCommand::QFLAG: break;
  case qCommand::QPIGS: if(sendCommand("QPIGS")) onPIGS(); break; //vervollständigen!
  case qCommand::QMOD: if(sendCommand("QMOD")) onMOD(); break; //vollständig
  case qCommand::QPIWS: break;
  case qCommand::QDI: break;
  case qCommand::QMCHGCR: if(sendCommand("QMCHGCR")) onMCHGCR();break; //vollständig
  case qCommand::QMUCHGCR: if(sendCommand("QMUCHGCR")) onMUCHGCR();break; //vollständig
  case qCommand::QBOOT: break;
  case qCommand::QOPM: break;
  }
}

/*
Sent Command: QMCHGCR
   Recived: (010 020 030 040 050 060 070 080 090 100 110 120
Sent Command: QMUCHGCR
   Recived: (002 010 020 030 040 050 060

*/

void sendMNCHGC(int val) //set  max AC + Solar Charge
{
  for (uint8_t i = 0; i <= (sizeof(_qchgcrMessage.chargeModes) / sizeof(_qchgcrMessage.chargeModes[0])); i++)
  {
    if (_qchgcrMessage.chargeModes[i] > val && _qchgcrMessage.chargeModes[i] != 0)
    {
      if (_qchgcrMessage.chargeModes[i-1] < 100)
      {
        sendCommand(("MNCHGC0" + String(_qchgcrMessage.chargeModes[i-1])));
      }
      else if (_qchgcrMessage.chargeModes[i-1] >= 100)
      {
        sendCommand(("MNCHGC" + String(_qchgcrMessage.chargeModes[i-1])));
      }
      break;
    }
  }
}

void sendMUCHGC(int val) //set mac AC Charge
{
  for (uint8_t i = 0; i <= (sizeof(_qchgcrMessage.uChargeModes) / sizeof(_qchgcrMessage.uChargeModes[0])); i++)
  {
    if (_qchgcrMessage.uChargeModes[i] > val && _qchgcrMessage.uChargeModes[i] != 0)
    {
      if (_qchgcrMessage.uChargeModes[i-1] <= 2)
      {
        sendCommand(("MUCHGC00" + String(_qchgcrMessage.uChargeModes[i-1])));
      }
      else if (_qchgcrMessage.uChargeModes[i-1] <= 99)
      {
        sendCommand(("MUCHGC0" + String(_qchgcrMessage.uChargeModes[i-1])));
      }
      break;
    }
  }
}
