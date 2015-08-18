//Handles talking to Thingspeak API
#include "inverter.h"
#include <ESP8266WiFi.h>
#include "settings.h"
#include "thingspeak.h"
#include "tickcounter.h"

#ifndef FPSTR
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

#define THINGSPEAK_IP IPAddress(184,106,153,149)
#define THINGSPEAK_ROLLOVER_MILLIS (24 * 3600 * 1000) //1 day

extern WiFiClient client;
extern Settings _settings;
extern bool _allMessagesUpdated;
extern QpiMessage _qpiMessage;
extern QpigsMessage _qpigsMessage;
extern QmodMessage _qmodMessage;
extern QpiwsMessage _qpiwsMessage;
extern QflagMessage _qflagMessage;
extern QidMessage _qidMessage;
extern TickCounter _tickCounter;

PollDelay _thingspeakLastUpdated(_tickCounter);
char udpPacketBuffer[1000];
bool isClientConnected = false;

//Periodically send an update to the thingspeak API
void serviceThingspeak()
{
  if (isClientConnected)
  {
    if (client.available())
    {
      char c = client.read();
      Serial.print(c);
    }
    
    if (!client.connected())
    {
      Serial.println();
      Serial.println("Client disconnected");
      client.stop();
      isClientConnected = false;
    }
  }

  //Check holdoff time
  if (_thingspeakLastUpdated.compare(_settings._updateRateSec * 1000) < 0)
  {
    return;
  }

  //Check if the previous batch of messages was received
  if (_allMessagesUpdated)
  { 
    updateThingspeakChargeApi();
    updateThingspeakBatteryApi();
    updateThingspeakLoadApi();
  }
}

//POST an update to thingspeak
bool updateThingspeak(const char* apiKey, String& params)
{
  if (apiKey == 0)
    return false;

  if (apiKey[0] == 0)
    return false;
   
  client.stop();

  Serial.println("Connecting");
  
  if (!client.connect(THINGSPEAK_IP, 80)) 
  {
    Serial.println("Can't connect to thingspeak");
    isClientConnected = false;
    return false;
  }

  Serial.println("Connected");
  
  isClientConnected = true;
  
  String getStr = F("POST /update HTTP/1.1\r\n");
  
  getStr += F("Host: api.thingspeak.com\r\n");
  
  getStr += F("Connection: close\r\n");
  
  getStr += F("X-THINGSPEAKAPIKEY: ");
  getStr += apiKey;
  getStr += F("\r\n");
  
  getStr += F("Content-Type: application/x-www-form-urlencoded\r\n");
  
  getStr += F("Content-Length: ");
  int len = params.length();
  getStr += String(len);
  getStr += F("\r\n");
  
  getStr += F("\r\n");
  
  getStr += params;

  client.println(getStr);
  client.println();

  Serial.println("Sent");
  

  Serial.println(getStr);

  _thingspeakLastUpdated.reset();
  
  return true;
}

const char statusStr[] PROGMEM = "status=";
const char field1Str[] PROGMEM = "&field1=";
const char field2Str[] PROGMEM = "&field2=";
const char field3Str[] PROGMEM = "&field3=";
const char field4Str[] PROGMEM = "&field4=";
const char field5Str[] PROGMEM = "&field5=";
const char field6Str[] PROGMEM = "&field6=";
const char field7Str[] PROGMEM = "&field7=";
const char field8Str[] PROGMEM = "&field8=";

String getWarningsText()
{
  String msg;

  /*
      reserved0                  0
      inverterFault              1
      busOver                    2
      busUnder                   3
      busSoftFail                4
      lineFail                   5
      opvShort                   6
      overTemperature            7
      fanLocked                  8
      batteryVoltageHigh         9
      batteryLowAlarm            A
      reserved13                 B 
      batteryUnderShutdown       C
      reserved15                 D 
      overload                   E
      eepromFault                F
      inverterOverCurrent        G
      inverterSoftFail           H
      selfTestFail               I
      opDcVoltageOver            J
      batOpen                    K
      currentSensorFail          L
      batteryShort               M
      powerLimit                 N
      pvVoltageHigh              O
      mpptOverloadFault          P
      mpptOverloadWarning        Q
      batteryTooLowToCharge      R
      reserved30                 S
      reserved31                 T
   */

  byte* p = (byte*)&_qpiwsMessage;
  for (int i=0; i<32; i++)
    if (p[i] == true)
      msg += (char)((i < 10) ? ('0' + i) : ('A' - 10 + i));

  return msg;
}

void updateThingspeakChargeApi()
{
  String params;
  String msg;
  
  //msg += "PI=";
  //msg += String(_qpiMessage.protocolId);
  msg += "mode=";
  msg += _qmodMessage.mode;
  msg += ",warn=";
  msg += getWarningsText();
  
  params += FPSTR(statusStr);
  params += msg;
  params += FPSTR(field1Str);
  params += String(_qpigsMessage.battV);
  params += FPSTR(field2Str); 
  params += String(_qpigsMessage.battChargeA);
  params += FPSTR(field3Str);
  params += String(_qpigsMessage.solarV);
  params += FPSTR(field4Str);
  params += String(_qpigsMessage.solarA);
  params += FPSTR(field5Str);
  params += String(_qpigsMessage.chargingStatus);
 
  updateThingspeak(_settings._chargerApiKey.c_str(), params);
}

void updateThingspeakBatteryApi()
{
  String params;
  String msg;
  
  //msg += "PI=";
  //msg += String(_qpiMessage.protocolId);
  msg += "mode=";
  msg += _qmodMessage.mode;
  msg += ",warn=";
  msg += getWarningsText();
  
  params += FPSTR(statusStr);
  params += msg;
  params += FPSTR(field1Str);
  params += String(_qpigsMessage.battV);
  params += FPSTR(field2Str); 
  params += String(_qpigsMessage.battChargeA);
  params += FPSTR(field3Str);
  params += String(_qpigsMessage.battDischargeA);
  params += FPSTR(field4Str);
  params += String(_qpigsMessage.battDischargeA);
 
  updateThingspeak(_settings._batteryApiKey.c_str(), params);
    
}

void updateThingspeakLoadApi()
{
  String params;
  String msg;
  
  //msg += "PI=";
  //msg += String(_qpiMessage.protocolId);
  msg += "mode=";
  msg += _qmodMessage.mode;
  msg += ",warn=";
  msg += getWarningsText();
  
  params += FPSTR(statusStr);
  params += msg;
  params += FPSTR(field1Str);
  params += String(_qpigsMessage.acOutV);
  params += FPSTR(field2Str); 
  params += String(_qpigsMessage.acOutHz);
  params += FPSTR(field3Str);
  params += String(_qpigsMessage.acOutW);
  params += FPSTR(field4Str);
  params += String(_qpigsMessage.acOutVa);
  params += FPSTR(field4Str);
  params += String(_qpigsMessage.heatSinkDegC);
 
  updateThingspeak(_settings._loadApiKey.c_str(), params);
    
  
}



