//Handles talking to Thingspeak API
#include "inverter.h"
#include <ESP8266WiFi.h>
#include "settings.h"
#include "thingspeak.h"

extern WiFiClient client;
extern Settings _settings;
extern bool _allMessagesUpdated;
extern QpiMessage _qpiMessage;
extern QpigsMessage _qpigsMessage;
extern QmodMessage _qmodMessage;
extern QpiwsMessage _qpiwsMessage;
extern QflagMessage _qflagMessage;
extern QidMessage _qidMessage;

#define THINGSPEAK_IP IPAddress(184,106,153,149)
#define THINGSPEAK_ROLLOVER_MILLIS (24 * 3600 * 1000) //1 day

char udpPacketBuffer[1000];
unsigned long thingspeakLastUpdated = 0;

bool isClientConnected = false;

unsigned long getMillisSinceLastThingspeakUpdate()
{
  if (thingspeakLastUpdated == 0)
    return 0xFFFFFFFF;
  
  return (millis() - thingspeakLastUpdated) & 0x7FFFFFFF;
}

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

  //Reset the thingspeakLastUpdated counter after a day so that rollover won't be a problem
  if (thingspeakLastUpdated != 0)
  {
    if (getMillisSinceLastThingspeakUpdate() > THINGSPEAK_ROLLOVER_MILLIS)
      thingspeakLastUpdated = 0;
  }

  if (_allMessagesUpdated)
  {
    updateThingspeakChargeApi();
  }
}

bool updateThingspeak(const char* apiKey, String& params)
{
  //Check holdoff time
  if (getMillisSinceLastThingspeakUpdate() < _settings._updateRateSec)
  {
    //Serial.println("Wait a bit before sending!");
    return false;
  }
    
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

  Serial.println(getStr);

  thingspeakLastUpdated = millis();
  if (thingspeakLastUpdated == 0) thingspeakLastUpdated = 1;

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

void updateThingspeakChargeApi()
{
  String params = "";

  params += statusStr;
  params += "Test Status";
  params += field1Str;
  params += String(_qpigsMessage.battV);
  params += field2Str; 
  params += String(_qpigsMessage.battChargeA);
  params += field3Str;
  params += String(_qpigsMessage.solarV);
  params += field4Str;
  params += String(_qpigsMessage.solarA);
  params += field5Str;
  params += String(_qpigsMessage.chargingStatus);
  
  updateThingspeak(_settings._chargerApiKey.c_str(), params);
}

void updateThingspeakBatteryApi(QpigsMessage& qpigs, String& statusText)
{
  String params = "";
  params += "f1=";
  params += String(qpigs.battV);
  params += "&f2=";
  params += String(qpigs.battChargeA);
  params += "&f3=";
  params += String(qpigs.battDischargeA); //Avg in future
  params += "&f4=";
  params += String(qpigs.battDischargeA); //Max in future
  params += "&status=";
  params += statusText;
}

void updateThingspeakLoadApi(QpigsMessage& qpigs, String& statusText)
{
  String params = "";
  params += "f1=";
  params += String(qpigs.acOutV);
  params += "&f2=";
  params += String(qpigs.acOutHz);
  params += "&f3=";
  params += String(qpigs.acOutW); 
  params += "&f4=";
  params += String(qpigs.acOutVa);
  params += "&f5=";
  params += String(qpigs.heatSinkDegC);
  params += "&status=";
  params += statusText;
}



