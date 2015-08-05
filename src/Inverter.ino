#define INT16U unsigned int
#define INT8U byte

struct QpigsMessage
{
  unsigned long rxTimeSec;
  float gridV;
  float gridHz;
  float acOutV;
  float acOutHz;
  short acOutVa;
  short acOutW;
  byte acOutPercent;
  short busV;
  float battV;
  float battChargeA;
  float battPercent;
  float reservedP;
  float solarA;
  float solarV;
  float reservedS;
  float reservedT;
  bool reservedU7;
  bool reservedU6;
  bool isSccFirmwareUpdated;
  bool isLoadOn; 
  bool reservedU3;
  byte chargingStatus;
  byte reservedY;
  byte reservedZ;
  long reservedAA;
  short reservedBB;
}; 

String battApiKey = "ZQZVTOCAQTYW2EB9";
String chargerApiKey = "LI18WECZ0GDYCC9R";
String loadApiKey = "GAHO2S5KRMZOZUXK";

String commandBuffer;
String lastRequestedCommand = "";
QpigsMessage qpigsMessage = {0};

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

//Parse the response to QPIGS general status message
void onPIGS()
{
  Serial.print("'");
  Serial.print(commandBuffer);
  Serial.print("'");

  int index = 1; //after the starting '('
  qpigsMessage.rxTimeSec = _tickCounter.getSeconds();
  qpigsMessage.gridV = getNextFloat(commandBuffer, index);
  qpigsMessage.gridHz = getNextFloat(commandBuffer, index);
  qpigsMessage.acOutV = getNextFloat(commandBuffer, index);
  qpigsMessage.acOutHz = getNextFloat(commandBuffer, index);
  qpigsMessage.acOutVa = (short)getNextLong(commandBuffer, index);
  qpigsMessage.acOutW = (short)getNextLong(commandBuffer, index);
  qpigsMessage.acOutPercent = (byte)getNextLong(commandBuffer, index);
  qpigsMessage.busV = (short)getNextLong(commandBuffer, index);
  qpigsMessage.battV = getNextFloat(commandBuffer, index);
  qpigsMessage.battChargeA = getNextFloat(commandBuffer, index);
  qpigsMessage.battPercent = getNextFloat(commandBuffer, index);
  qpigsMessage.reservedP = getNextFloat(commandBuffer, index);
  qpigsMessage.solarA = getNextFloat(commandBuffer, index);
  qpigsMessage.solarV = getNextFloat(commandBuffer, index);
  qpigsMessage.reservedS = getNextFloat(commandBuffer, index);
  qpigsMessage.reservedT = getNextFloat(commandBuffer, index);
  qpigsMessage.reservedU7 = getNextBit(commandBuffer, index);
  qpigsMessage.reservedU6 = getNextBit(commandBuffer, index);
  qpigsMessage.isSccFirmwareUpdated = getNextBit(commandBuffer, index);
  qpigsMessage.isLoadOn = getNextBit(commandBuffer, index);             
  qpigsMessage.reservedU3 = getNextBit(commandBuffer, index);
  qpigsMessage.chargingStatus = (byte)getNextLong(commandBuffer, index);
  qpigsMessage.reservedY = (byte)getNextLong(commandBuffer, index);
  qpigsMessage.reservedZ = (byte)getNextLong(commandBuffer, index);
  qpigsMessage.reservedAA = getNextLong(commandBuffer, index);
  qpigsMessage.reservedBB = (short)getNextLong(commandBuffer, index);

  String msg = "";
  msg += "field1=";
  msg += String(qpigsMessage.battV);
  msg += "&field2=";
  msg += String(qpigsMessage.battChargeA);
  msg += "&field3=";
  msg += String(qpigsMessage.solarV);
  msg += "&field4=";
  msg += String(qpigsMessage.solarA);
  
  updateThingspeak(chargerApiKey.c_str(), msg.c_str());
  
  Serial.println(qpigsMessage.rxTimeSec);
  Serial.println(qpigsMessage.gridV);
  Serial.println(qpigsMessage.gridHz);
  Serial.println(qpigsMessage.acOutV);
  
  
}

void onInverterCommand()
{
  if ((commandBuffer.length() > 3) && (commandBuffer[0] == '('))
  {
    unsigned short calculatedCrc = cal_crc_half((byte*)commandBuffer.c_str(), commandBuffer.length() - 2);
    unsigned short recievedCrc = ((unsigned short)commandBuffer[commandBuffer.length()-2] << 8) | 
                                                  commandBuffer[commandBuffer.length()-1];

    Serial.print(" Calc: ");
    Serial.print(calculatedCrc, HEX);
    Serial.print(" Rx: ");
    Serial.println(recievedCrc, HEX);
    
    if (calculatedCrc == recievedCrc)
    {
      if (lastRequestedCommand == "QPIGS") onPIGS();
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
      if (commandBuffer.length() < 255)
        commandBuffer += (char)c;
    }
    else if ((c == '\r') || (c == '\n'))
    {
      onInverterCommand();
      commandBuffer = "";
    }
  }   
}

void requestInverterCommand(String command)
{
  Serial.println(command);
  
  unsigned short crc = cal_crc_half((byte*)command.c_str(), command.length());

  lastRequestedCommand = command;
  
  Serial1.print(command);
  Serial1.print((char)((crc >> 8) & 0xFF));
  Serial1.print((char)((crc >> 0) & 0xFF));
  Serial1.print("\r\n");
}

