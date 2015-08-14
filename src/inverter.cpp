#define INT16U unsigned int
#define INT8U byte

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
String _lastRequestedCommand = "";
QpigsMessage _qpigsMessage = {0};
QmodMessage _qmodMessage = {0};


//Found here: http://forums.aeva.asn.au/pip4048ms-inverter_topic4332_post53760.html#53760
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
void onPIGS()
{
  Serial.print("'");
  Serial.print(_commandBuffer);
  Serial.print("'");

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

  String msg = "";
  msg += "field1=";
  msg += String(_qpigsMessage.battV);
  msg += "&field2=";
  msg += String(_qpigsMessage.battChargeA);
  msg += "&field3=";
  msg += String(_qpigsMessage.solarV);
  msg += "&field4=";
  msg += String(_qpigsMessage.solarA);
  
  updateThingspeak(_settings._chargerApiKey.c_str(), msg.c_str());
  
  Serial.println(_qpigsMessage.rxTimeSec);
  Serial.println(_qpigsMessage.gridV);
  Serial.println(_qpigsMessage.gridHz);
  Serial.println(_qpigsMessage.acOutV);
}

//Parse the response to QMOD general status message, CRC has already been confirmed
void onMOD()
{
  Serial.print("'");
  Serial.print(_commandBuffer);
  Serial.print("'");

  
}

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
    
    if (calculatedCrc == recievedCrc)
    {
      if (_lastRequestedCommand == "QPIGS") onPIGS();
      if (_lastRequestedCommand == "QMOD") onMOD();
      
    }
  }
}

void serviceInverter()
{
  byte c;
    
  while (SerialRx.read(c))
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

void requestInverterCommand(String command)
{
  Serial.println(command);
  
  unsigned short crc = cal_crc_half((byte*)command.c_str(), command.length());

  _lastRequestedCommand = command;
  
  Serial1.print(command);
  Serial1.print((char)((crc >> 8) & 0xFF));
  Serial1.print((char)((crc >> 0) & 0xFF));
  Serial1.print("\r\n");
}

