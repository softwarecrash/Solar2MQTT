/*************************************************************************************
/* ALl credits for some bits from https://github.com/scottwday/InverterOfThings
/* And i have trashed alot of his code, rewritten some. So thanks to him and credits to him. 
/* Changes by softwarecrash
/*************************************************************************************/

#include <Wire.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include "TickCounter.h"
#include "inverter.h"
#include <ESP8266WebServer.h>
#include "Settings.h"
#include "weblog.h" //coming soon to view the serial log in the webpages

#include "webpages/HTMLcase.h"     //The HTML Konstructor
#include "webpages/main.h"         //landing page with menu
#include "webpages/livedata.h"     //live data page
#include "webpages/settings.h"     //settings page
#include "webpages/mqttsettings.h" //mqtt settings page

WiFiClient client;
Settings _settings;
WebLog webLog;
TickCounter _tickCounter;

extern QpigsMessage _qpigsMessage;
extern QmodMessage _qmodMessage;
extern QetMessage _qetMessage; //new testing
extern QpiriMessage _qpiriMessage; //new testing
extern P003GSMessage _P003GSMessage;
extern P003PSMessage _P003PSMessage;
extern P006FPADJMessage _P006FPADJMessage;
extern String _nextCommandNeeded;
extern String _setCommand;
extern String _otherBuffer;

PubSubClient mqttclient(client);

// Interface types that can be used.
const byte PCM = 0; //not sure it works
const byte MPI = 1; //not sure it works
const byte PIP = 2; 
byte inverterType = PIP; //And defaults in case...
String topic = "/";      //Default first part of topic. We will add device ID in setup
String st = "";
String ajaxStr = "";

unsigned long mqtttimer = 0;
extern bool _allMessagesUpdated;
extern bool _otherMessagesUpdated;
//---------- LEDS
int Led_Red = 5;   //D1
int Led_Green = 4; //D2

StaticJsonDocument<300> doc;
StaticJsonDocument<350> ajaxJs;
ESP8266WebServer server(80);

//flag for saving data
bool shouldSaveConfig = false;
char mqtt_server[40];
//----------------------------------------------------------------------
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup()
{
  _settings.load();
  delay(1000);
  WiFiManager wm; // global wm instance
  wm.setSaveConfigCallback(saveConfigCallback);

  Wire.begin(4, 5);
  Serial1.begin(9600); // Debugging towards UART1
  Serial.begin(2400);  // Using UART0 for comm with inverter. IE cant be connected during flashing

  //for testing list the saved data on Serial 1
  Serial1.println();
  Serial1.printf("Device Name:\t");
  Serial1.println(_settings._deviceName);
  Serial1.printf("Mqtt Server:\t");
  Serial1.println(_settings._mqttServer);
  Serial1.printf("Mqtt Port:\t");
  Serial1.println(_settings._mqttPort);
  Serial1.printf("Mqtt User:\t");
  Serial1.println(_settings._mqttUser);
  Serial1.printf("Mqtt Passwort:\t");
  Serial1.println(_settings._mqttPassword);
  Serial1.printf("Mqtt Interval:\t");
  Serial1.println(_settings._mqttRefresh);
  Serial1.printf("Mqtt Topic:\t");
  Serial1.println(_settings._mqttTopic);

  //set the device name
  WiFi.hostname(_settings._deviceName);
  //create custom wifimanager fields
  WiFiManagerParameter custom_mqtt_server ("mqtt_server", "MQTT server",        NULL, 40);
  WiFiManagerParameter custom_mqtt_user   ("mqtt_user",   "MQTT User",          NULL, 40);
  WiFiManagerParameter custom_mqtt_pass   ("mqtt_pass",   "MQTT Password",      NULL, 65);
  WiFiManagerParameter custom_mqtt_topic  ("mqtt_topic",  "MQTT Topic",         NULL, 60);
  WiFiManagerParameter custom_mqtt_port   ("mqtt_port",   "MQTT Port",          NULL, 5);
  WiFiManagerParameter custom_mqtt_refresh("mqtt_refresh","MQTT Send Interval", NULL, 5);
  WiFiManagerParameter custom_device_name ("device_name", "Device Name",        NULL, 40);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);

  bool res = wm.autoConnect("Solar-AP");

  wm.setConnectTimeout(30);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds

  //save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
    _settings._mqttServer = custom_mqtt_server.getValue();
    _settings._mqttUser = custom_mqtt_user.getValue();
    _settings._mqttPassword = custom_mqtt_pass.getValue();
    _settings._mqttPort = atoi(custom_mqtt_port.getValue());
    _settings._deviceName = custom_device_name.getValue();
    _settings._mqttTopic = custom_mqtt_topic.getValue();
    _settings._mqttRefresh = atoi(custom_mqtt_refresh.getValue());
    _settings.save();
    delay(1500);
    _settings.load();
    ESP.restart();
  }

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

  topic = _settings._mqttTopic;

  mqttclient.setServer(_settings._mqttServer.c_str(), _settings._mqttPort);

  mqttclient.setCallback(callback);

  pinMode(Led_Red, OUTPUT);
  pinMode(Led_Green, OUTPUT);
  digitalWrite(Led_Red, HIGH);
  digitalWrite(Led_Green, LOW);
  //check is WiFi connected
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
      server.send(200, "text/html", sendHTMLlive());
    });

    server.on("/reboot", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Location", String("/"), true);
      server.send(302, "text/plain", "");
      ESP.restart();
    });

    server.on("/reset", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Location", String("/"), true);
      server.send(302, "text/plain", "");
      _settings.reset();
      ESP.eraseConfig();
      ESP.restart();
    });

    server.on("/settings", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", sendHTMLsettings());
    });

    server.on("/mqttsettings", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", sendHTMLmqttsettings());
    });

    //ajax part
    server.on("/livedataAjax", HTTP_GET, []() {
      ajaxJsUpdate();
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
  }
}
//end void setup

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void loop()
{
  delay(100);
  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  { //No use going to next step unless WIFI is up and running.

    server.handleClient();
    //httpServer.handleClient();
    MDNS.update();

    //If we have pending data to send send it first!
    //sendRaw();
    // Comms with inverter
    serviceInverter(); // Check if we recieved data or should send data
    sendtoMQTT();      // Update data to MQTT server if we should

    // Check if we have something to read from MQTT
    mqttclient.loop();
  }
}
//End void loop




void ajaxJsUpdate()
{ //update the json doc for the ajax request
  ajaxJs.clear();
  ajaxJs["gridV"] = _qpigsMessage.gridV;
  ajaxJs["gridHz"] = _qpigsMessage.gridHz;

  ajaxJs["acOutV"] = _qpigsMessage.acOutV;
  ajaxJs["acOutHz"] = _qpigsMessage.acOutHz;
  ajaxJs["acOutVa"] = _qpigsMessage.acOutVa;
  ajaxJs["acOutW"] = _qpigsMessage.acOutW;
  ajaxJs["acOutPercent"] = _qpigsMessage.acOutPercent;

  ajaxJs["busV"] = _qpigsMessage.busV;

  ajaxJs["heatSinkDegC"] = _qpigsMessage.heatSinkDegC;

  ajaxJs["battV"] = _qpigsMessage.battV;
  ajaxJs["battPercent"] = _qpigsMessage.battPercent;
  ajaxJs["battChargeA"] = _qpigsMessage.battChargeA;
  ajaxJs["battDischargeA"] = _qpigsMessage.battDischargeA;
  ajaxJs["sccBattV"] = _qpigsMessage.sccBattV;

  ajaxJs["solarV"] = _qpigsMessage.solarV;
  ajaxJs["solarA"] = _qpigsMessage.solarA;
  ajaxJs["solarW"] = _qpigsMessage.solarW;

  ajaxJs["cSOC"] = _qpigsMessage.cSOC;

  ajaxJs["iv_mode"] = _qmodMessage.operationMode;



  ajaxStr = "";
  serializeJson(ajaxJs, ajaxStr);
  server.send(200, "text/plane", ajaxStr); //Send value only to client ajax request
  Serial1.println("Ajax Request answer:");
  Serial1.println(ajaxStr);
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
  if (millis() < (mqtttimer + (_settings._mqttRefresh * 1000))) //its save for rollover?
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

      mqttclient.publish((topic + String("/Device Data/IP")).c_str(), String(WiFi.localIP().toString()).c_str());
 
    }
    else
    {
      Serial1.println(F("CANT CONNECT TO MQTT"));
      digitalWrite(Led_Green, LOW);
      //delay(50);
      return false; // Exit if we couldnt connect to MQTT brooker
    }
  }

  //mqttclient.publish((topic + String("/wifi")).c_str(), (String("{ \"FreeRam\": ") + String(ESP.getFreeHeap()) + String(", \"rssi\": ") + String(WiFi.RSSI()) + String(", \"dbm\": ") + String(WifiGetRssiAsQuality(WiFi.RSSI())) + String("}")).c_str());

  Serial1.print(F("Data sent to MQTT SERver"));
  Serial1.print(F(" - up: "));
  digitalWrite(Led_Green, HIGH);




  if (!_allMessagesUpdated)
    return false;

  _allMessagesUpdated = false; // Lets reset messages and process them
                               
  if (inverterType == PIP) { //orginal PCM
     mqttclient.publish((String(topic) + String("/battv")).c_str(), String(_qpigsMessage.battV).c_str());
     mqttclient.publish((String(topic) + String("/solarv")).c_str(), String(_qpigsMessage.solarV).c_str());
     mqttclient.publish((String(topic) + String("/batta")).c_str(), String(_qpigsMessage.battChargeA).c_str());
     mqttclient.publish((String(topic) + String("/wattage")).c_str(), String(_qpigsMessage.acOutW).c_str());
     mqttclient.publish((String(topic) + String("/solara")).c_str(), String(_qpigsMessage.solarA).c_str());

    doc.clear();
    doc["battvtest"] =  _qpigsMessage.battV;
    doc["solarv"] = _qpigsMessage.solarV;
    doc["batta"] =  _qpigsMessage.battChargeA;
    doc["wattage"] =_qpigsMessage.acOutW;
    doc["solara"] = _qpigsMessage.solarA;
    st = "";
    serializeJson(doc,st);
    mqttclient.publish((String(topic) + String("/status")).c_str(), st.c_str() );
  }

  if (inverterType == PCM) //orginal PIP
  {
    mqttclient.publish((String(topic) + String("/Grid Voltage")).c_str(), String(_qpigsMessage.gridV).c_str());
    mqttclient.publish((String(topic) + String("/Grid Frequenz")).c_str(), String(_qpigsMessage.gridHz).c_str());

    mqttclient.publish((String(topic) + String("/AC out Voltage")).c_str(), String(_qpigsMessage.acOutV).c_str());
    mqttclient.publish((String(topic) + String("/AC out Frequenz")).c_str(), String(_qpigsMessage.acOutHz).c_str());
    mqttclient.publish((String(topic) + String("/AC out VA")).c_str(), String(_qpigsMessage.acOutVa).c_str());
    mqttclient.publish((String(topic) + String("/AC out Watt")).c_str(), String(_qpigsMessage.acOutW).c_str());
    mqttclient.publish((String(topic) + String("/AC out percent")).c_str(), String(_qpigsMessage.acOutPercent).c_str());

    mqttclient.publish((String(topic) + String("/Bus Volt")).c_str(), String(_qpigsMessage.busV).c_str());
    mqttclient.publish((String(topic) + String("/Bus Temp")).c_str(), String(_qpigsMessage.heatSinkDegC).c_str());

    mqttclient.publish((String(topic) + String("/Battery Voltage")).c_str(), String(_qpigsMessage.battV).c_str());
    mqttclient.publish((String(topic) + String("/Battery Percent")).c_str(), String(_qpigsMessage.battPercent).c_str());
    mqttclient.publish((String(topic) + String("/Battery Charge A")).c_str(), String(_qpigsMessage.battChargeA).c_str());
    mqttclient.publish((String(topic) + String("/Battery Discharge A")).c_str(), String(_qpigsMessage.battDischargeA).c_str());
    mqttclient.publish((String(topic) + String("/Battery SCC Volt")).c_str(), String(_qpigsMessage.sccBattV).c_str());

    mqttclient.publish((String(topic) + String("/PV Volt")).c_str(), String(_qpigsMessage.solarV).c_str());
    mqttclient.publish((String(topic) + String("/PV A")).c_str(), String(_qpigsMessage.solarA).c_str());
    mqttclient.publish((String(topic) + String("/PV Watt")).c_str(), String(_qpigsMessage.solarW).c_str());

    mqttclient.publish((String(topic) + String("/Inverter Operation Mode")).c_str(), String(_qmodMessage.operationMode).c_str());
    //Beta
    mqttclient.publish((String(topic) + String("/Calculated SOC")).c_str(), String(_qpigsMessage.cSOC).c_str());


    //for testing, not sure, i dont recive this data
    if(_qetMessage.energy > 0)mqttclient.publish((String(topic) + String("/Energy/Total Energy KWh")).c_str(), String(_qetMessage.energy).c_str());

    //piri answer
    mqttclient.publish((String(topic) + String("/Device Data/Grid rating voltage")).c_str(), String(_qpiriMessage.gridRatingV).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/Grid rating current")).c_str(), String(_qpiriMessage.gridRatingA).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/AC output rating voltage")).c_str(), String(_qpiriMessage.acOutV).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/Grid rating frequency")).c_str(), String(_qpiriMessage.gridRatingHz).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/AC output rating current")).c_str(), String(_qpiriMessage.acOutA).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/Grid rating Watt")).c_str(), String(_qpiriMessage.gridRatingW).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/AC rating Watt")).c_str(), String(_qpiriMessage.acOutRatingW).c_str());
    mqttclient.publish((String(topic) + String("/Device Data/Battery rating voltage")).c_str(), String(_qpiriMessage.battRatingV).c_str());


    //doc.clear();
    //doc["pBattV"] = _qpigsMessage.battV;
    //doc["battPercent"] = _qpigsMessage.battPercent;
    //st = "";
    //serializeJson(doc, st);
    //mqttclient.publish((String(topic) + String("/status")).c_str(), st.c_str());
    
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
    //mqttclient.publish((String(topic) + String("/debug/recieved")).c_str(), String(_otherBuffer).c_str());
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

  //mqttclient.publish((String(topic) + String("/debug/sent")).c_str(), String("top: " + topic + " data: " + st).c_str());
  Serial1.print(F("Current command: "));
  Serial1.print(_nextCommandNeeded);
  Serial1.print(F(" Setting next command to : "));
  Serial1.println(st);
  _setCommand = st;
  // Add code to put the call into the queue but verify it firstly. Then send the result back to debug/new window?
}
