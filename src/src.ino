//TODO:
// Use keys from settings for upload

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FileSystem.h>
#include <ip_addr.h>
#include <espconn.h>
#include <EEPROM.h>
#include <EspSoftSerialRx.h>

#include "main.h"
#include "TickCounter.h"
#include "Settings.h"
#include "inverter.h"
#include "thingspeak.h"
  
Settings _settings;
TickCounter _tickCounter;
PollDelay _inverterDelay(_tickCounter);

//////////////////////
// WiFi Definitions //
//////////////////////
struct espconn connection;
const char ApSsid[] = "Setup";

/////////////////////
// Pin Definitions //
/////////////////////



const int SOFTSERIAL_TX = 2;
const int SOFTSERIAL_RX = 12;
EspSoftSerialRx SerialRx;

//--------------------------------- LEDs

const int RED_LED_PIN = 14; // red LED
const int GRN_LED_PIN = 13; // green LED
const int RED_BTN_PIN = 0; // red btn
const int GRN_BTN_PIN = 15; //12; // green btn



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
byte counter = 0;

WiFiClient client;

//----------------------------------------------------------------------
void setup() 
{
  initHardware();
  _settings.load();
  serviceWifiMode();
  server.begin();
  
}

//----------------------------------------------------------------------
void loop() 
{
  delay(50);
  if (!counter) counter = 100;
  --counter;

  if (digitalRead(RED_BTN_PIN) == 0)
  {
    requestApMode = WIFI_AP;
    //requestInverterCommand("QPIGS");
  }
 
  if (_inverterDelay.compare(16000) > 0)
  {
    _inverterDelay.reset();
    requestInverterCommand("QPIGS");
  }

  // Make sure wifi is in the right mode
  serviceWifiMode();  

  serviceThingspeak();

  serviceLeds();

  //serviceSoftSerialRx();
  SerialRx.service();

  serviceInverter();
  
  
  //If we're connected to an AP then send updates
  if ((currentApMode == WIFI_STA) && (clientConnectionState == CLIENT_CONNECTED))
  {
//    if (getMillisSinceLastThingspeakUpdate() > 30000)
//      updateThingspeakTest1(temperatureDegC, pressureMb); 
  }
  
  
  // Check if a client has connected
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

  //drawLcd();

  delay(1);
  
  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}



void initHardware()
{
  delay(100);
  Wire.begin(4, 5);
  
  pinMode(RED_BTN_PIN, INPUT_PULLUP);
  pinMode(GRN_BTN_PIN, INPUT_PULLUP);

  initLeds();

  //setupLcd();
  
  Serial.begin(9600);
  delay(10);

  //Second uart uses hardware transmitter TX1 and software interrupt receiver
  pinMode(SOFTSERIAL_TX, OUTPUT);
  digitalWrite(SOFTSERIAL_TX, HIGH);
  Serial1.begin(2400);
  SerialRx.begin(2400, SOFTSERIAL_RX);

  
}

