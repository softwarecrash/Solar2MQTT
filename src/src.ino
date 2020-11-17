
//TODO:
// Fix WIFI
// Fix setting of inverter type
// Add set via mqtt?


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <espconn.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include "uptime_formatter.h"


#include "main.h"
#include "TickCounter.h"
#include "Settings.h"
#include "inverter.h"


const char ApSsid[] = "Setup";

/////////////////////
// Pin Definitions //
/////////////////////

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
int WIFI_COUNT = 0;

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
const byte PCM = 0;
const byte PIP = 2;

#define dev "pcm2"
byte inverterType = PCM;  // 0 for pip and 1 for MPI


#define topic "solar/" dev

unsigned long mqtttimer = 0;
  
//----------------------------------------------------------------------
void setup() 
{
  initHardware();
  _settings.load();
  //serviceWifiMode();
  setup_wifi();
  
  server.begin();
  delay(100);
  
  mqttclient.setServer(mqttServer, mqttPort);
    if (mqttclient.connect((String("ESP-" +ESP.getChipId())).c_str(), mqttUser, mqttPassword )) {
      Serial1.println("connected to MQTT SERVER");
    } else
      Serial1.println("Couldnt connect MQTT right now. Will try later");
}

extern bool _allMessagesUpdated;

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void loop() 
{
  delay(50);
   // requestApMode = WIFI_AP;  // Left in case of need. This setups the ap mode to read
  // Make sure wifi is in the right mode
  //serviceWifiMode();  

  // Comms with inverter
  serviceInverter();
  sendtoMQTT();
  
  // Check if a client towards port 80 has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial1.println(req);
  client.flush();

  if (req.indexOf("/wifi") != -1)
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
  
  Serial1.begin(115200); // Debugging towards UART1
  Serial.begin(2400); // Using UART0 for comm with inverter. IE cant be connected during flashing

}

int WifiGetRssiAsQuality(int rssi)  // THis part borrowed from Tasmota code
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}


bool sendtoMQTT() {

    if (millis() < (mqtttimer + 4000)) { 
    return false;
  }
  mqtttimer = millis();
  
  Serial1.print("Wifi status: ");
  Serial1.println(WiFi.status());
  if (WiFi.status() == 1) {
    Serial1.println("Lets disconnect WIFI to test");
    WiFi.disconnect();
    delay(300);
    setup_wifi();
  }
    
  if (!mqttclient.connected()) {
    if (mqttclient.connect((String("ESP-" +ESP.getChipId())).c_str(), mqttUser, mqttPassword )) {
        Serial1.println("Reconnected to MQTT SERVER");
        mqttclient.publish((String("stat/") + String(dev) + String("/info")).c_str(), "Im alive!");
      } else {
        Serial1.println("CANT CONNECT TO MQTT");
        return false; // Exit if we couldnt connect to MQTT brooker
      }
  } 
    mqttclient.publish((String("stat/") + String(dev) + String("/uptime")).c_str(), String(uptime_formatter::getUptime()).c_str());
    mqttclient.publish((String("stat/") + String(dev) + String("/wifi")).c_str(),   (String("{ \"FreeRam\": ") + String(ESP.getFreeHeap()) + String(", \"rssi\": ") + String(WiFi.RSSI()) + String(", \"dbm\": ") + String(WifiGetRssiAsQuality(WiFi.RSSI())) + String("}")).c_str());  
    Serial1.print("Data sent to MQTT SERver");
    Serial1.println(" - up: " + uptime_formatter::getUptime());

  
  if (!_allMessagesUpdated) return false;
  
  _allMessagesUpdated = false; // Lets reset messages and process them
  
  if (inverterType == PCM) { //PCM
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



  return true;
}


//** ALL BELOW IS TEMP TO SEE IF WIFI SHIT IS CRAP

void setup_wifi() {
  delay(10);
  Serial1.println("");
  Serial1.print("Connecting to ");
  Serial1.print(_settings._wifiSsid);
  //WiFi.hostname(_settings._wifiPass);

  //WiFi.begin(_settings._wifiSsid.c_str(), _settings._wifiPass.c_str());
  WiFi.begin("Esperyd", "Esperyd4");
  while (WiFi.status() != WL_CONNECTED) {
    Serial1.print(WiFi.status());
    yield();
    delay(1500);
    Serial1.print(".");
    if (WiFi.status() == 1) {
      Serial1.println("Lets disconnect WIFI to test");
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
        ETS_UART_INTR_DISABLE();
  wifi_station_disconnect();
  ETS_UART_INTR_ENABLE();
      delay(15000);
      //ESP.restart();
      WiFi.begin(_settings._wifiSsid.c_str(), _settings._wifiPass.c_str());
    }
  }
  Serial1.println("OK");
  Serial1.print("   IP address: ");
  Serial1.println(WiFi.localIP());
}
