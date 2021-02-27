
/*************************************************************************************
/* ALl credits for some bits from https://github.com/scottwday/InverterOfThings
/* And i have trashed alot of his code, rewritten some. So thanks to him and credits to him. 
/*
/* Changes done by Daniel aka Daromer aka DIY Tech & Repairs 2020
/* https://github.com/daromer2/InverterOfThings
/* https://www.youtube.com/channel/UCI6ASwT150rendNc5ytYYrQ?
/*************************************************************************************/

//TODO:
// Clean up webpages
// Fix update timer?
// Rewrite send to MQTT part so it sends json perhaps?

// Add some code so we can set stuff on the inverters.
// MPI should have the feedToGridCorrection so it can be set via mqtt

#include <Wire.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include "main.h"
#include "TickCounter.h"
#include "Settings.h"
#include "inverter.h"
#include <ESP8266WebServer.h>

#include <Ticker.h>
Ticker callInverter;

#include "webpages/HTMLcase.h"
#include "webpages/main.h"
#include "webpages/livedata.h"
#include "webpages/settings.h"

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
WiFiClient client;
Settings _settings;
TickCounter _tickCounter;
int WIFI_COUNT = 0;

extern QpigsMessage _qpigsMessage;
extern P003GSMessage _P003GSMessage;
extern P003PSMessage _P003PSMessage;
extern P006FPADJMessage _P006FPADJMessage;
extern String _nextCommandNeeded;
extern String _setCommand;
extern String _otherBuffer;

//---------------------- MQTT
PubSubClient mqttclient(client);

// Interface types that can be used.
const byte MPI = 1;
const byte PCM = 0;
const byte PIP = 2;
byte inverterType = PCM;  //And defaults in case...
String topic ="/"; //Default first part of topic. We will add device ID in setup
String st = "";

unsigned long mqtttimer = 0;
extern bool _allMessagesUpdated;
extern bool _otherMessagesUpdated;
//---------- LEDS
int Led_Red = 5;   //D1
int Led_Green = 4; //D2

StaticJsonDocument<300> doc;

ESP8266WebServer server(80);

WiFiManager wm; // global wm instance

//----------------------------------------------------------------------
void setup()
{
  //callInverter.attach(10, serviceInverter); //call the inverter every 10 seconds


  Wire.begin(4, 5);
  Serial1.begin(9600); // Debugging towards UART1
  Serial.begin(2400);    // Using UART0 for comm with inverter. IE cant be connected during flashing

  _settings.load();
  delay(2500);

  Wire.begin(4, 5);
  Serial1.begin(9600); // Debugging towards UART1
  Serial.begin(2400);  // Using UART0 for comm with inverter. IE cant be connected during flashing

  WiFi.hostname(_settings._deviceName);
  WiFiManagerParameter mqtt_server("mqtt_server", "MQTT server", NULL, 40);
  WiFiManagerParameter mqtt_user("mqtt_user", "MQTT User", NULL, 40);
  WiFiManagerParameter mqtt_pass("mqtt_pass", "MQTT Password", NULL, 40);
  WiFiManagerParameter mqtt_topic("mqtt_topic", "MQTT Topic", NULL, 20);
  WiFiManagerParameter mqtt_port("mqtt_port", "MQTT Port", NULL, 5);

  wm.addParameter(&mqtt_server);
  wm.addParameter(&mqtt_user);
  wm.addParameter(&mqtt_pass);
  wm.addParameter(&mqtt_topic);
  wm.addParameter(&mqtt_port);
  bool res;
  res = wm.autoConnect("Solar-AP");

  wm.setConnectTimeout(20);      // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(60); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  //wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  if (String(_settings._deviceType) == "MPI")
  {
    inverterType = MPI;
  }
  else if (String(_settings._deviceType) == "PIP")
  {
    inverterType = PIP;
  }
  else
  {
    inverterType = PCM;
  }

  //dev = _settings._deviceName.c_str();
  topic = topic + String(_settings._deviceName.c_str());

  mqttclient.setServer(_settings._mqttServer.c_str(), _settings._mqttPort);
  mqttclient.setCallback(callback);

  pinMode(Led_Red, OUTPUT);
  pinMode(Led_Green, OUTPUT);
  digitalWrite(Led_Red, HIGH);
  digitalWrite(Led_Green, LOW);

  if (!res)
  {
    Serial1.println("Failed to connect or hit timeout");
  }
  else
  {

    MDNS.begin(_settings._deviceName);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", sendHTMLmain());
    });

    server.on("/livedata", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Refresh", "5");
      server.send(200, "text/html", sendHTMLlive());
    });

    server.on("/reboot", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Location", String("/"), true);
      server.send(302, "text/plain", "");
      delay(2000);
      ESP.restart();
    });

    server.on("/reset", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Location", String("/"), true);
      server.send(302, "text/plain", "");
      delay(2000);
      ESP.eraseConfig();
      ESP.restart();
    });

    server.on("/settings", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", sendHTMLsettings());
    });

    //part for the web update
    server.on(
        "/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Location", String("/"), true);
      server.send ( 302, "text/plain", "");
      delay(2000);
      ESP.restart(); }, []() {
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial1.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial1.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial1.setDebugOutput(false);
      }
      yield(); });
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial1.println("Webserver Running...");
    Serial1.println("MQTT Settings:");
    Serial1.printf("Server: ");
    Serial1.println(mqtt_server.getValue()); 
    Serial1.printf("User: ");
    Serial1.println(mqtt_user.getValue());
    Serial1.printf("Password: ");
    Serial1.println(mqtt_pass.getValue());
    Serial1.printf("Topic: ");
    Serial1.println(mqtt_topic.getValue());
  }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void loop()
{
  delay(100);
  server.handleClient();
  // Make sure wifi is in the right mode
  //serviceWifiMode();
  if (WiFi.status() == WL_CONNECTED)
  { //No use going to next step unless WIFI is up and running.

    //httpServer.handleClient();
    MDNS.update();

    //If we have pending data to send send it first!
    //sendRaw();
    // Comms with inverter
    serviceInverter(); // Check if we recieved data or should send data
    sendtoMQTT();      // Update data to MQTT server if we should

    // Check if we have something to read from MQTT
    mqttclient.loop();
    //return;
  }
  // Check if a client towards port 80 has connected
  //WiFiClient client = server.client.;
 // if (!client)
 // {
 //   return;
 // }

  // Read the first line of the request
  //String req = client.readStringUntil('\r');
  //Serial1.println(req);
  //client.flush();

}



void processingdata(){
  if (WiFi.status() == WL_CONNECTED)
  { //No use going to next step unless WIFI is up and running.

    //If we have pending data to send send it first!
    //sendRaw();
    // Comms with inverter
    serviceInverter(); // Check if we recieved data or should send data
    sendtoMQTT();      // Update data to MQTT server if we should
  _allMessagesUpdated = false;
    // Check if we have something to read from MQTT
    mqttclient.loop();
    return;
  }
  String req = client.readStringUntil('\r');
  Serial1.println(req);
  client.flush();
}



int WifiGetRssiAsQuality(int rssi) // THis part borrowed from Tasmota code
{
  int quality = 0;

  if (rssi <= -100)
  {
    quality = 0;
  }
  else if (rssi >= -50)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

bool sendtoMQTT()
{
  if (millis() < (mqtttimer + 3000))
  {
    return false;
  }
  mqtttimer = millis();
  if (!mqttclient.connected())
  {
    //delete the esp name string
    if (mqttclient.connect((String(_settings._deviceName)).c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str()))
    {

      Serial1.println(F("Reconnected to MQTT SERVER"));

      mqttclient.publish((topic + String("/Info")).c_str(), ("{\"Status\":\"Im alive!\", \"DeviceType\": \"" + _settings._deviceType + "\",\"IP \":\"" + WiFi.localIP().toString() + "\"}").c_str());
      mqttclient.subscribe((topic + String("/code")).c_str());
      mqttclient.subscribe((topic + String("/code")).c_str());
    }
    else
    {
      Serial1.println(F("CANT CONNECT TO MQTT"));
      digitalWrite(Led_Green, LOW);
      //delay(50);
      return false; // Exit if we couldnt connect to MQTT brooker
    }
  }

  //mqttclient.publish((topic + String("/uptime")).c_str(), String("{\"human\":\"" + String(uptime_formatter::getUptime()) + "\", \"seconds\":" + String(millis() / 1000) + "}").c_str());
  mqttclient.publish((topic + String("/wifi")).c_str(), (String("{ \"FreeRam\": ") + String(ESP.getFreeHeap()) + String(", \"rssi\": ") + String(WiFi.RSSI()) + String(", \"dbm\": ") + String(WifiGetRssiAsQuality(WiFi.RSSI())) + String("}")).c_str());

  Serial1.print(F("Data sent to MQTT SERver"));
  Serial1.print(F(" - up: "));
  //Serial1.println(uptime_formatter::getUptime());
  digitalWrite(Led_Green, HIGH);

  if (!_allMessagesUpdated)
    return false;

  _allMessagesUpdated = false; // Lets reset messages and process them
                               /*
  if (inverterType == PCM) { //PCM
     mqttclient.publish((String(topic) + String("/battv")).c_str(), String(_qpigsMessage.battV).c_str());
     mqttclient.publish((String(topic) + String("/solarv")).c_str(), String(_qpigsMessage.solarV).c_str());
     mqttclient.publish((String(topic) + String("/batta")).c_str(), String(_qpigsMessage.battChargeA).c_str());
     mqttclient.publish((String(topic) + String("/wattage")).c_str(), String(_qpigsMessage.wattage).c_str());
     mqttclient.publish((String(topic) + String("/solara")).c_str(), String(_qpigsMessage.solarA).c_str());

    doc.clear();
    doc["battvtest"] =  _qpigsMessage.battV;
    doc["solarv"] = _qpigsMessage.solarV;
    doc["batta"] =  _qpigsMessage.battChargeA;
    doc["wattage"] =_qpigsMessage.wattage;
    doc["solara"] = _qpigsMessage.solarA;
    st = "";
    serializeJson(doc,st);
    mqttclient.publish((String(topic) + String("/status")).c_str(), st.c_str() );

     
  }*/

  if (inverterType == PCM)
  { //PIP changed to pcm
    mqttclient.publish((String(topic) + String("/Grid Voltage")).c_str(), String(_qpigsMessage.gridV).c_str());
    mqttclient.publish((String(topic) + String("/Grid Frequenz")).c_str(), String(_qpigsMessage.gridHz).c_str());

    mqttclient.publish((String(topic) + String("/AC out Voltage")).c_str(), String(_qpigsMessage.acOutV).c_str());
    mqttclient.publish((String(topic) + String("/AC out Frequenz")).c_str(), String(_qpigsMessage.acOutHz).c_str());
    mqttclient.publish((String(topic) + String("/AC out VA")).c_str(), String(_qpigsMessage.acOutVa).c_str());
    mqttclient.publish((String(topic) + String("/AC out Watt")).c_str(), String(_qpigsMessage.acOutW).c_str());
    mqttclient.publish((String(topic) + String("/AC out percent")).c_str(), String(_qpigsMessage.acOutPercent).c_str());

    mqttclient.publish((String(topic) + String("/Buss Volt")).c_str(), String(_qpigsMessage.busV).c_str());

    mqttclient.publish((String(topic) + String("/Battery Voltage")).c_str(), String(_qpigsMessage.battV).c_str());
    mqttclient.publish((String(topic) + String("/Battery Percent")).c_str(), String(_qpigsMessage.battPercent).c_str());
    mqttclient.publish((String(topic) + String("/Battery Charge A")).c_str(), String(_qpigsMessage.battChargeA).c_str());
    mqttclient.publish((String(topic) + String("/Battery Discharge A")).c_str(), String(_qpigsMessage.battDischargeA).c_str());
    mqttclient.publish((String(topic) + String("/Battery SCC Volt")).c_str(), String(_qpigsMessage.sccBattV).c_str());

    mqttclient.publish((String(topic) + String("/PV Volt")).c_str(), String(_qpigsMessage.solarV).c_str());
    mqttclient.publish((String(topic) + String("/PV Watt")).c_str(), String(_qpigsMessage.solarA).c_str());

    doc.clear();
    doc["pBattV"] = _qpigsMessage.battV;
    doc["battPercent"] = _qpigsMessage.battPercent;
    st = "";
    serializeJson(doc, st);
    mqttclient.publish((String(topic) + String("/status")).c_str(), st.c_str());
  }

  if (inverterType == MPI)
  { //IF MPI
    mqttclient.publish((String(topic) + String("/solar1w")).c_str(), String(_P003GSMessage.solarInputV1 * _P003GSMessage.solarInputA1).c_str());
    mqttclient.publish((String(topic) + String("/solar2w")).c_str(), String(_P003GSMessage.solarInputV2 * _P003GSMessage.solarInputA2).c_str());
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

    doc.clear();
    doc["solar1w"] = _P003GSMessage.solarInputV1 * _P003GSMessage.solarInputA1;
    doc["solar2w"] = _P003GSMessage.solarInputV2 * _P003GSMessage.solarInputA2;
    doc["solarInputV1"] = _P003GSMessage.solarInputV1;
    doc["solarInputV2"] = _P003GSMessage.solarInputV2;
    doc["solarInputA1"] = _P003GSMessage.solarInputA1;
    doc["solarInputA2"] = _P003GSMessage.solarInputA2;
    doc["battV"] = _P003GSMessage.battV;
    doc["battA"] = _P003GSMessage.battA;
    doc["acInputVoltageR"] = _P003GSMessage.acInputVoltageR;
    doc["acInputVoltageS"] = _P003GSMessage.acInputVoltageS;
    doc["acInputVoltageT"] = _P003GSMessage.acInputVoltageT;
    doc["acInputCurrentR"] = _P003GSMessage.acInputCurrentR;
    doc["acInputCurrentS"] = _P003GSMessage.acInputCurrentS;
    doc["acInputCurrentT"] = _P003GSMessage.acInputCurrentT;
    doc["acOutputCurrentR"] = _P003GSMessage.acOutputCurrentR;
    doc["acOutputCurrentS"] = _P003GSMessage.acOutputCurrentS;
    doc["acOutputCurrentT"] = _P003GSMessage.acOutputCurrentT;
    doc["acWattageR"] = _P003PSMessage.w_r;
    doc["acWattageS"] = _P003PSMessage.w_s;
    doc["acWattageT"] = _P003PSMessage.w_t;
    doc["acWattageTotal"] = _P003PSMessage.w_total;
    doc["acOutputProcentage"] = _P003PSMessage.ac_output_procent;
    doc["feedingGridDirectionR"] = _P006FPADJMessage.feedingGridDirectionR;
    doc["feedingGridDirectionS"] = _P006FPADJMessage.feedingGridDirectionS;
    doc["feedingGridDirectionT"] = _P006FPADJMessage.feedingGridDirectionT;
    doc["calibrationWattR"] = _P006FPADJMessage.calibrationWattR;
    doc["calibrationWattS"] = _P006FPADJMessage.calibrationWattS;
    doc["calibrationWattT"] = _P006FPADJMessage.calibrationWattT;
    st = "";
    serializeJson(doc, st);
    mqttclient.publish((String(topic) + String("/status")).c_str(), st.c_str());
  }

  return true;
}

// Check if we have pending raw messages to send to MQTT. Then send it.
void sendRaw()
{
  if (_otherMessagesUpdated)
  {
    _otherMessagesUpdated = false;
    Serial1.print("Sending other data to mqtt: ");
    Serial1.println(_otherBuffer);
    mqttclient.publish((String(topic) + String("/debug/recieved")).c_str(), String(_otherBuffer).c_str());
  }
}
/// TESTING MQTT SEND

void callback(char *top, byte *payload, unsigned int length)
{
  Serial1.println(F("Callback done"));
  if (strcmp(top, "pir1Status") == 0)
  {
    // whatever you want for this topic
  }

  String st = "";
  for (int i = 0; i < length; i++)
  {
    st += String((char)payload[i]);
  }

  mqttclient.publish((String(topic) + String("/debug/sent")).c_str(), String("top: " + topic + " data: " + st).c_str());
  Serial1.print(F("Current command: "));
  Serial1.print(_nextCommandNeeded);
  Serial1.print(F(" Setting next command to : "));
  Serial1.println(st);
  _setCommand = st;
  // Add code to put the call into the queue but verify it firstly. Then send the result back to debug/new window?
}
