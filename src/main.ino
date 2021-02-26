#include <Wire.h>
//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
//#include <FS.h>
//#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "settings.h"
#include "inverter.h"
#include "TickCounter.h"

//webpages
#include "webpages/HTMLcase.h"
#include "webpages/main.h"
#include "webpages/livedata.h"
#include "webpages/settings.h"

TickCounter _tickCounter;
//Settings _settings;
const char *hostName = "Solar2MQTT";

//---------- LEDS
int Led_Red = 5;   //D1
int Led_Green = 4; //D2

// Interface types that can be used.
const byte MPI = 1;
const byte PCM = 0;
const byte PIP = 2;
byte inverterType = PCM; //And defaults in case...

extern bool _allMessagesUpdated;
extern bool _otherMessagesUpdated;

extern QpigsMessage _qpigsMessage;
extern P003GSMessage _P003GSMessage;
extern P003PSMessage _P003PSMessage;
extern P006FPADJMessage _P006FPADJMessage;
extern String _nextCommandNeeded;
extern String _setCommand;
extern String _otherBuffer;

ESP8266WebServer server(80);

WiFiManager wm; // global wm instance

void setup(void)
{

  //_settings.setupSpiffs();

  Wire.begin(4, 5);
  Serial1.begin(9600); // Debugging towards UART1
  Serial.begin(2400);  // Using UART0 for comm with inverter. IE cant be connected during flashing

  WiFi.hostname(hostName);
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

  if (!res)
  {
    Serial1.println("Failed to connect or hit timeout");
  }
  else
  {

    MDNS.begin(hostName);
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

void loop(void)
{

  delay(1500);
  server.handleClient();
  delay(1);

  // Comms with inverter
  serviceInverter(); // Check if we recieved data or should send data

  //if (!_allMessagesUpdated) return false;
  _allMessagesUpdated = false; // Lets reset messages and process them
}
