//Handles talking to Thingspeak API
#include "inverter.h"
#include <ESP8266WiFi.h>
#include "settings.h"

extern WiFiClient client;
extern Settings _settings;

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
}

bool updateThingspeak(const char* apiKey, const char* params)
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
  
  String getStr = "GET /update?key=";
  getStr += apiKey;
  getStr += "&";
  getStr += params;

  client.println(getStr);
  client.println();

  Serial.println(getStr);

  thingspeakLastUpdated = millis();
  if (thingspeakLastUpdated == 0) thingspeakLastUpdated = 1;

  return true;
}

void updateThingspeakTest1(double temperature, double pressure)
{
  String params = "";
  params += "field1=";
  params += String(temperature);
  params += "&field2=";
  params += String(pressure);
  
  updateThingspeak("PRH5FVNM8819MIBL", params.c_str());
  
}

void updateThingspeakChargeApi(QpigsMessage& qpigs, String& statusText)
{
  String params = "";
  params += "f1=";
  params += String(qpigs.battV);
  params += "&f2=";
  params += String(qpigs.battChargeA);
  params += "&f3=";
  params += String(qpigs.solarV);
  params += "&f4=";
  params += String(qpigs.solarA);
  params += "&f5=";
  params += String(qpigs.chargingStatus);
  params += "&status=";
  params += statusText;
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



