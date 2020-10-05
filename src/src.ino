
//TODO:
// Use keys from settings for upload

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
//#include <ip_addr.h>
#include <espconn.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include "uptime_formatter.h"

#include "EspSoftSerialRx.h" //Copied from: https://github.com/scottwday/EspSoftSerial
#include "main.h"
#include "TickCounter.h"
#include "Settings.h"
#include "inverter.h"
//#include "thingspeak.h"

const char ApSsid[] = "Setup";

/////////////////////
// Pin Definitions //
/////////////////////
const int SOFTSERIAL_TX = 2;
const int SOFTSERIAL_RX = 12;
const int RED_LED_PIN = 14;   // red LED
const int GRN_LED_PIN = 13;   // green LED
const int RED_BTN_PIN = 0;    // red btn
const int GRN_BTN_PIN = 15;   //12; // green btn

#define LED_MODE_SOLID_BOTH 0xFF
#define LED_MODE_SOLID_RED 0xAA
#define LED_MODE_SOLID_GRN 0x55
#define LED_MODE_SLOW_RED 0xA0;
#define LED_MODE_SLOW_GRN 0x50;
#define LED_MODE_FAST_RED 0x44;
#define LED_MODE_FAST_GRN 0x11;
#define LED_DELAY 4
byte ledMode = 0;
byte ledState = 0;
byte ledCounter = 0;

//--------------------------------- Wifi State
#define CLIENT_NOTCONNECTED 0
#define CLIENT_RECONNECT 1
#define CLIENT_CONNECTING 2
#define CLIENT_PRECONNECTED 3
#define CLIENT_CONNECTED 4
byte currentApMode = 0;
byte requestApMode = WIFI_STA;
byte clientConnectionState = CLIENT_NOTCONNECTED;
bool clientReconnect = false;

//--------------------------------- IP connection
WiFiServer server(80);
WiFiClient client;
Settings _settings;
TickCounter _tickCounter;
EspSoftSerialRx SerialRx;

extern QpigsMessage _qpigsMessage;
extern P003GSMessage _P003GSMessage;
extern P003PSMessage _P003PSMessage;
extern P006FPADJMessage _P006FPADJMessage;

//---------------------- MQTT SHould be moved to setup
const char* mqttServer = "mqtt.romer.se";
const int mqttPort = 1883;
const char* mqttUser = "mqtt";
const char* mqttPassword = "mqtt";
PubSubClient mqttclient(client);

// Interface types that can be used. 
const byte MPI = 1;
const byte PCM60x = 0;
const byte PIP = 2;

#define dev "mpi"
byte inverterType = MPI;  // 0 for pip and 1 for MPI


#define topic "solar/" dev
  
//----------------------------------------------------------------------
void setup() 
{
  initHardware();
  _settings.load();
  serviceWifiMode();

  server.begin();
  delay(100);
  
  mqttclient.setServer(mqttServer, mqttPort);
    if (mqttclient.connect((String("ESP-" +ESP.getChipId())).c_str(), mqttUser, mqttPassword )) {
      Serial.println("connected to MQTT SERVER");
    } else
      Serial.println("Couldnt connect MQTT right now. Will try later");
}

extern bool _allMessagesUpdated;

//----------------------------------------------------------------------
void loop() 
{
  delay(50);

  if (digitalRead(RED_BTN_PIN) == 0)
  {
    requestApMode = WIFI_AP;
  }

  // Make sure wifi is in the right mode
  serviceWifiMode();  

  // Do the blinkenlights
  //serviceLeds();

  // Get any extra leftover chars from soft serial receiver
  SerialRx.service();

  // Comms with inverter
  serviceInverter();

  if (_allMessagesUpdated)
  {
    sendtoMQTT();  // Lets send all data when messages are done
    _allMessagesUpdated = false;
    delay(3000);
  }
  
  // Check if a client towards port 80 has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  if (req.indexOf("/thingspeak") != -1)
  {
    serveThingspeakSetupPage(client);    
  }
  else if (req.indexOf("/settskeys") != -1)
  {
    serveSetThingspeakKeys(client, req);    
  }
  else if (req.indexOf("/wifi") != -1)
  {
    serveWifiSetupPage(client);    
  }
  else if (req.indexOf("/aplist") != -1)
  {
    serveWifiApList(client);
  }
  else if (req.indexOf("/setap") != -1)
  {
    serveWifiSetAp(client, req);
  }
  else
  {
    serve404(client);
  }
  client.flush();
}

void initHardware()
{
 
  delay(100);
  Wire.begin(4, 5);
  
  pinMode(RED_BTN_PIN, INPUT_PULLUP);
  pinMode(GRN_BTN_PIN, INPUT_PULLUP);

  initLeds();

  //setupLcd();
  
  Serial.begin(115200);
  delay(10);

  //Second uart uses hardware transmitter TX1 and software interrupt receiver
  pinMode(SOFTSERIAL_TX, OUTPUT);
  digitalWrite(SOFTSERIAL_TX, HIGH);
  Serial1.begin(2400);
  SerialRx.begin(2400, SOFTSERIAL_RX);

}

bool sendtoMQTT() {
  if (!mqttclient.connected()) {
    if (mqttclient.connect((String("ESP-" +ESP.getChipId())).c_str(), mqttUser, mqttPassword )) {
        Serial.println("Reconnected to MQTT SERVER");
        mqttclient.publish((String("stat/") + String(dev) + String("/info")).c_str(), "Im alive!");
      } else return false; // Exit if we couldnt connect to MQTT brooker
  } 
  
  if (inverterType == PCM60x) { //PCM
     mqttclient.publish((String(topic) + String("/battv")).c_str(), String(_qpigsMessage.battV).c_str());
     mqttclient.publish((String(topic) + String("/solarv")).c_str(), String(_qpigsMessage.solarV).c_str());
     mqttclient.publish((String(topic) + String("/batta")).c_str(), String(_qpigsMessage.battChargeA).c_str());
     mqttclient.publish((String(topic) + String("/wattage")).c_str(), String(_qpigsMessage.wattage).c_str());
     mqttclient.publish((String(topic) + String("/solara")).c_str(), String(_qpigsMessage.solarA).c_str());
  }
  if (inverterType == MPI) { //IF MPI
    mqttclient.publish((String(topic) + String("/solar1w")).c_str(), String(_P003GSMessage.solarInputV1*_P003GSMessage.solarInputA1).c_str());
    mqttclient.publish((String(topic) + String("/solar2w")).c_str(), String(_P003GSMessage.solarInputV2*_P003GSMessage.solarInputA2).c_str());
    mqttclient.publish((String(topic) + String("/solarInputV1")).c_str(), String(_P003GSMessage.solarInputV1).c_str());
    mqttclient.publish((String(topic) + String("/solarInputV2")).c_str(), String(_P003GSMessage.solarInputV2).c_str());
    mqttclient.publish((String(topic) + String("/solarInputA1")).c_str(), String(_P003GSMessage.solarInputA1).c_str());
    mqttclient.publish((String(topic) + String("/solarInputA2")).c_str(), String(_P003GSMessage.solarInputA2).c_str()); 
    mqttclient.publish((String(topic) + String("/battV")).c_str(), String(_P003GSMessage.battV).c_str());
    mqttclient.publish((String(topic) + String("/battA")).c_str(), String(_P003GSMessage.battA).c_str());

    mqttclient.publish((String(topic) + String("/acInputVoltageR")).c_str(), String(_P003GSMessage.acInputVoltageR).c_str());   
    mqttclient.publish((String(topic) + String("/acInputVoltageS")).c_str(), String(_P003GSMessage.acInputVoltageS).c_str());
    mqttclient.publish((String(topic) + String("/acInputVoltageT")).c_str(), String(_P003GSMessage.acInputVoltageT).c_str());

    mqttclient.publish((String(topic) + String("/acInputCurrentR")).c_str(), String(_P003GSMessage.acInputCurrentR).c_str());   
    mqttclient.publish((String(topic) + String("/acInputCurrentS")).c_str(), String(_P003GSMessage.acInputCurrentS).c_str());
    mqttclient.publish((String(topic) + String("/acInputCurrentT")).c_str(), String(_P003GSMessage.acInputCurrentT).c_str());

    mqttclient.publish((String(topic) + String("/acOutputCurrentR")).c_str(), String(_P003GSMessage.acOutputCurrentR).c_str());   
    mqttclient.publish((String(topic) + String("/acOutputCurrentS")).c_str(), String(_P003GSMessage.acOutputCurrentS).c_str());
    mqttclient.publish((String(topic) + String("/acOutputCurrentT")).c_str(), String(_P003GSMessage.acOutputCurrentT).c_str());

    mqttclient.publish((String(topic) + String("/acWattageR")).c_str(), String(_P003PSMessage.w_r).c_str());
    mqttclient.publish((String(topic) + String("/acWattageS")).c_str(), String(_P003PSMessage.w_s).c_str());
    mqttclient.publish((String(topic) + String("/acWattageT")).c_str(), String(_P003PSMessage.w_t).c_str());
    mqttclient.publish((String(topic) + String("/acWattageTotal")).c_str(), String(_P003PSMessage.w_total).c_str());
    mqttclient.publish((String(topic) + String("/ac_output_procent")).c_str(), String(_P003PSMessage.ac_output_procent).c_str());

    mqttclient.publish((String(topic) + String("/feedingGridDirectionR")).c_str(), String(_P006FPADJMessage.feedingGridDirectionR).c_str());
    mqttclient.publish((String(topic) + String("/calibrationWattR")).c_str(), String(_P006FPADJMessage.calibrationWattR).c_str());
    mqttclient.publish((String(topic) + String("/feedingGridDirectionS")).c_str(), String(_P006FPADJMessage.feedingGridDirectionS).c_str());
    mqttclient.publish((String(topic) + String("/calibrationWattS")).c_str(), String(_P006FPADJMessage.calibrationWattS).c_str());
    mqttclient.publish((String(topic) + String("/feedingGridDirectionT")).c_str(), String(_P006FPADJMessage.feedingGridDirectionT).c_str());
    mqttclient.publish((String(topic) + String("/calibrationWattT")).c_str(), String(_P006FPADJMessage.calibrationWattT).c_str());
  
  }


  mqttclient.publish((String("stat/") + String(dev) + String("/uptime")).c_str(), String(uptime_formatter::getUptime()).c_str());
  Serial.print("Data sent to MQTT SERver");
  Serial.println(" - up: " + uptime_formatter::getUptime());
  return true;
}
