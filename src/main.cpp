/*
Solar2MQTT Project
https://github.com/softwarecrash/Solar2MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.


Dear programmer:
When I wrote this code, only god and
I knew how it worked.
Now, only god knows it!

Therefore, if you are trying to optimize
this routine and it fails (most surely),
please increase this counter as a
warning for the next person:

total_hours_wasted_here = 254
*/
#include "main.h"

#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Settings.h"

#include "webpages/htmlCase.h"     //The HTML Konstructor
#include "webpages/main.h"         //landing page with menu
#include "webpages/settings.h"     //settings page
#include "webpages/settingsedit.h" //mqtt settings page

#include "PI_Serial/PI_Serial.h"

PI_Serial mppClient(INVERTER_RX, INVERTER_TX);

WiFiClient client;
PubSubClient mqttclient(client);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
Settings _settings;

// new importetd
char mqttClientId[80];
ADC_MODE(ADC_VCC);

// flag for saving data
// String topic = "/"; // Default first part of topic. We will add device ID in setup
unsigned long mqtttimer = 0;
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool valChange = false;
bool askInverterOnce = false;
bool publishFirst = false;
String commandFromWeb;
String commandFromMqtt;
String customResponse;

bool firstPublish;

char mqttClientId[80];

StaticJsonDocument<JSON_BUFFER> Json;                          // main Json
JsonObject deviceJson = Json.createNestedObject("Device");     // basic device data
JsonObject staticData = Json.createNestedObject("DeviceData"); // battery package data
JsonObject liveData = Json.createNestedObject("liveData");     // battery package data
//----------------------------------------------------------------------
void saveConfigCallback()
{
  DEBUG_PRINTLN("Should save config");
  shouldSaveConfig = true;
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend())
  {
    DEBUG_PRINT(F("Info: Data sent to WebSocket... "));
    char data[JSON_BUFFER];
    size_t len = serializeJson(Json, data);
    wsClient->text(data, len);
    DEBUG_PRINTLN(F("Done"));
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsClient = client;
    //getJsonDevice();
    //getJsonData();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    wsClient = nullptr;
    break;
  case WS_EVT_DATA:
   // bmstimer = millis();
    mqtttimer = millis();
    //handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)
  {
    DEBUG_PRINTLN("Update");
    Update.runAsync(true);
    if (!Update.begin(free_space))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device is booting new Firmware");
      response->addHeader("Refresh", "10; url=/");
      response->addHeader("Connection", "close");
      request->send(response);
      restartNow = true; // Set flag so main loop can issue restart call
      DEBUG_PRINTLN("Update complete");
    }
  }
}

void setup()
{
#ifdef DEBUG
  DEBUG_BEGIN(9600); // Debugging towards UART1
#endif
  _settings.load();

  // mppClient.Init(); // init the PI_serial Library

  WiFi.persistent(true); // fix wifi save bug

  AsyncWiFiManager wm(&server, &dns);

  wm.setSaveConfigCallback(saveConfigCallback);

  sprintf(mqttClientId, "%s-%06X", _settings.data.deviceName, ESP.getChipId());

  DEBUG_PRINTLN();
  DEBUG_PRINTF("Device Name:\t");
  DEBUG_PRINTLN(_settings.data.deviceName);
  DEBUG_PRINTF("Mqtt Server:\t");
  DEBUG_PRINTLN(_settings.data.mqttServer);
  DEBUG_PRINTF("Mqtt Port:\t");
  DEBUG_PRINTLN(_settings.data.mqttPort);
  DEBUG_PRINTF("Mqtt User:\t");
  DEBUG_PRINTLN(_settings.data.mqttUser);
  DEBUG_PRINTF("Mqtt Passwort:\t");
  DEBUG_PRINTLN(_settings.data.mqttPassword);
  DEBUG_PRINTF("Mqtt Interval:\t");
  DEBUG_PRINTLN(_settings.data.mqttRefresh);
  DEBUG_PRINTF("Mqtt Topic:\t");
  DEBUG_PRINTLN(_settings.data.mqttTopic);

  // mppClient.setProtocol(mppClient.PI30_HS_MS_MSX); // manual set the protocol
  mppClient.Init(); // init the PI_serial Library

  // create custom wifimanager fields

  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 100);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", NULL, 30);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", NULL, 6);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", NULL, 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", NULL, 40);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);

  bool apRunning = wm.autoConnect("Solar-AP");

  wm.setConnectTimeout(30);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds

  // save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
    strncpy(_settings.data.mqttServer, custom_mqtt_server.getValue(), 40);
    strncpy(_settings.data.mqttUser, custom_mqtt_user.getValue(), 40);
    strncpy(_settings.data.mqttPassword, custom_mqtt_pass.getValue(), 40);
    _settings.data.mqttPort = atoi(custom_mqtt_port.getValue());
    strncpy(_settings.data.deviceName, custom_device_name.getValue(), 40);
    strncpy(_settings.data.mqttTopic, custom_mqtt_topic.getValue(), 40);
    _settings.data.mqttRefresh = atoi(custom_mqtt_refresh.getValue());
    _settings.save();
    ESP.restart();
  }

  mqttclient.setServer(_settings.data.mqttServer, _settings.data.mqttPort);
  mqttclient.setCallback(mqttcallback);
  mqttclient.setBufferSize(MQTT_BUFFER);

  // check is WiFi connected
  if (!apRunning)
  {
    DEBUG_PRINTLN("Failed to connect or hit timeout");
  }
  else
  {
    // deviceJson["IP"] = WiFi.localIP(); // grab the device ip

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_MAIN);
                response->printf_P(HTML_FOOT);
                request->send(response); });
    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument liveJson(1024);
                
                liveJson["gridV"] = mppClient.get.variableData.gridVoltage;
                
                liveJson["gridHz"] = mppClient.get.variableData.gridFrequency;
                liveJson["acOutV"] = mppClient.get.variableData.acOutputVoltage;
                liveJson["acOutHz"] = mppClient.get.variableData.acOutputFrequency;
                
                liveJson["acOutVa"] = mppClient.get.variableData.acOutputApparentPower;
                
                liveJson["acOutW"] = mppClient.get.variableData.acOutputActivePower;
                liveJson["acOutPercent"] = mppClient.get.variableData.outputLoadPercent;
                liveJson["busV"] = mppClient.get.variableData.busVoltage;
                liveJson["heatSinkDegC"] = mppClient.get.variableData.inverterHeatSinkTemperature;
                liveJson["battV"] = mppClient.get.variableData.batteryVoltage;
                liveJson["battPercent"] = mppClient.get.variableData.batteryCapacity;
                liveJson["battChargeA"] = mppClient.get.variableData.batteryChargingCurrent;
                liveJson["battDischargeA"] = mppClient.get.variableData.batteryDischargeCurrent;
                liveJson["sccBattV"] = mppClient.get.variableData.batteryVoltageFromScc;
                liveJson["solarV"] = mppClient.get.variableData.pvInputVoltage[0];
                liveJson["solarA"] = mppClient.get.variableData.pvInputCurrent[0];
                liveJson["solarW"] = mppClient.get.variableData.pvChargingPower; //not realy?
                liveJson["iv_mode"] = mppClient.get.variableData.operationMode;

                liveJson["device_name"] = _settings.data.deviceName;
                serializeJson(liveJson, *response);
                request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                restartNow = true; });
    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_CONFIRM_RESET);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                delay(1000);
                _settings.reset();
                ESP.eraseConfig();
                ESP.restart(); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("text/html");
                response->printf_P(HTML_HEAD);
                response->printf_P(HTML_SETTINGS_EDIT);
                response->printf_P(HTML_FOOT);
                request->send(response); });

    server.on("/settingsjson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument SettingsJson(256);
                SettingsJson["device_name"] = _settings.data.deviceName;
                SettingsJson["mqtt_server"] = _settings.data.mqttServer;
                SettingsJson["mqtt_port"] = _settings.data.mqttPort;
                SettingsJson["mqtt_topic"] = _settings.data.mqttTopic;
                SettingsJson["mqtt_user"] = _settings.data.mqttUser;
                SettingsJson["mqtt_password"] = _settings.data.mqttPassword;
                SettingsJson["mqtt_refresh"] = _settings.data.mqttRefresh;
                serializeJson(SettingsJson, *response);
                request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                request->redirect("/settings");
                strncpy(_settings.data.mqttServer, request->arg("post_mqttServer").c_str(), 40);
                _settings.data.mqttPort = request->arg("post_mqttPort").toInt();
                strncpy(_settings.data.mqttUser, request->arg("post_mqttUser").c_str(), 40);
                strncpy(_settings.data.mqttPassword, request->arg("post_mqttPassword").c_str(), 40);
                strncpy(_settings.data.mqttTopic, request->arg("post_mqttTopic").c_str(), 40);
                _settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prevent lower numbers
                strncpy(_settings.data.deviceName, request->arg("post_deviceName").c_str(), 40);
                _settings.save();
                request->redirect("/reboot"); });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "maxcharge")
                {
                  valChange = true;
                //  _qpiriMessage.battMaxChrgA = p->value().toInt(); //const string zu int
                }
                if (p->name() == "maxaccharge")
                {
                  valChange = true;
                 // _qpiriMessage.battMaxAcChrgA = p->value().toInt(); //const string zu int
                }
                if (p->name() == "PCVV")
                {
                  valChange = true;
                 // _qpiriMessage.battBulkV = p->value().toFloat(); //const string zu int
                }
                if (p->name() == "PBFT")
                {
                  valChange = true;
                  //_qpiriMessage.battFloatV = p->value().toFloat(); //const string zu int
                }

                if (p->name() == "CC")
                {
                  valChange = true;
                  commandFromWeb = (p->value());
                }
                request->send(200, "text/plain", "message received"); });

    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          request->send(200);
          request->redirect("/"); },
        handle_update_progress_cb);

    server.begin();
    MDNS.addService("http", "tcp", 80);

    DEBUG_PRINTLN("Webserver Running...");

    // set the device name
    MDNS.addService("http", "tcp", 80);
    if (MDNS.begin(_settings.data.deviceName))
      DEBUG_PRINTLN(F("mDNS running..."));
    WiFi.hostname(_settings.data.deviceName);
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
  }
}
// end void setup

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void loop()
{

  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  { // No use going to next step unless WIFI is up and running.
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();
    

    if (valChange)
    {
      if (commandFromWeb != "")
      {
        DEBUG_PRINTLN(commandFromWeb);
        String tmp = mppClient.sendCommand(commandFromWeb); // send a custom command to the device
        DEBUG_PRINTLN(tmp);
        commandFromWeb = "";
      }
      if (commandFromMqtt != "")
      {
        DEBUG_PRINTLN(commandFromMqtt);
        String customResponse = mppClient.sendCommand(commandFromMqtt); // send a custom command to the device
        DEBUG_PRINTLN(customResponse);
        commandFromMqtt = "";
        mqttclient.publish((String(_settings.data.mqttTopic) + String("/Device_Control/Set_Command_answer")).c_str(), customResponse.c_str());
      }

      mppClient.getStaticeData();

      mqtttimer = 0;
      valChange = false;
    }
    else
    {
      mppClient.getVariableData(); // später durch update ersetzen
      if (!askInverterOnce)
      {
        mppClient.getStaticeData();
        askInverterOnce = true;
      }
    }

    sendtoMQTT(); // Update data to MQTT server if we should

    mqttclient.loop(); // Check if we have something to read from MQTT
  }
  if (restartNow)
  {
    delay(1000);
    DEBUG_PRINTLN("Restart");
    ESP.restart();
  }
}
// End void loop

char *topicBuilder(char *buffer, char const *path, char const *numering = "")
{                                                   // buffer, topic
  const char *mainTopic = _settings.data.mqttTopic; // get the main topic path
  strcpy(buffer, mainTopic);
  strcat(buffer, "/");
  strcat(buffer, path);
  strcat(buffer, numering);
  return buffer;
}

bool connectMQTT()
{
  char buff[256];
  if (!mqttclient.connected())
  {
    firstPublish = false;
    DEBUG_PRINT(F("Info: MQTT Client State is: "));
    DEBUG_PRINTLN(mqttclient.state());
    DEBUG_PRINT(F("Info: establish MQTT Connection... "));

    if (mqttclient.connect(mqttClientId, _settings.data.mqttUser, _settings.data.mqttPassword, (topicBuilder(buff, "alive")), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {
        DEBUG_PRINTLN(F("Done"));
        mqttclient.publish(topicBuilder(buff, "alive"), "true", true); // LWT online message must be retained!
        mqttclient.publish(topicBuilder(buff, "Device_IP"), (const char *)(WiFi.localIP().toString()).c_str(), true);
        mqttclient.subscribe(topicBuilder(buff, "/Device_Control/Set_Command"));

        if (_settings.data.relaisFunction == 4)
          mqttclient.subscribe(topicBuilder(buff, "Device_Control/Relais"));
      }
      else
      {
        DEBUG_PRINT(F("Fail\n"));
      }
    }
    else
    {
      DEBUG_PRINT(F("Fail\n"));
      return false; // Exit if we couldnt connect to MQTT brooker
    }
    firstPublish = true;
  }
  return true;
}

bool sendtoMQTT()
{
  char msgBuffer[32];
  char buff[256]; // temp buffer for the topic string
  if (!connectMQTT())
  {
    DEBUG_PRINTLN(F("Error: No connection to MQTT Server, cant send Data!"));
    firstPublish = false;
    return false;
  }
  DEBUG_PRINT(F("Info: Data sent to MQTT Server... "));
  if (!_settings.data.mqttJson)
  {

   // mqttclient.publish(topicBuilder(buff, "Pack_Voltage"), dtostrf(bms.get.packVoltage, 4, 1, msgBuffer));
  //  mqttclient.publish(topicBuilder(buff, "Pack_Current"), dtostrf(bms.get.packCurrent, 4, 1, msgBuffer));
  //  mqttclient.publish(topicBuilder(buff, "Pack_Power"), dtostrf((bms.get.packVoltage * bms.get.packCurrent), 4, 1, msgBuffer));
  //  mqttclient.publish(topicBuilder(buff, "Pack_SOC"), dtostrf(bms.get.packSOC, 6, 2, msgBuffer));
   // mqttclient.publish(topicBuilder(buff, "Pack_Remaining_mAh"), itoa(bms.get.resCapacitymAh, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cycles"), itoa(bms.get.bmsCycles, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_BMS_Temperature"), itoa(bms.get.tempAverage, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cell_High"), itoa(bms.get.maxCellVNum, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cell_Low"), itoa(bms.get.minCellVNum, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cell_High_Voltage"), dtostrf(bms.get.maxCellmV / 1000, 5, 3, msgBuffer));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cell_Low_Voltage"), dtostrf(bms.get.minCellmV / 1000, 5, 3, msgBuffer));
   // mqttclient.publish(topicBuilder(buff, "Pack_Cell_Difference"), itoa(bms.get.cellDiff, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_ChargeFET"), bms.get.chargeFetState ? "true" : "false");
   // mqttclient.publish(topicBuilder(buff, "Pack_DischargeFET"), bms.get.disChargeFetState ? "true" : "false");
   // mqttclient.publish(topicBuilder(buff, "Pack_Status"), bms.get.chargeDischargeStatus);
   // mqttclient.publish(topicBuilder(buff, "Pack_Cells"), itoa(bms.get.numberOfCells, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Heartbeat"), itoa(bms.get.bmsHeartBeat, msgBuffer, 10));
   // mqttclient.publish(topicBuilder(buff, "Pack_Balance_Active"), bms.get.cellBalanceActive ? "true" : "false");

   // for (size_t i = 0; i < bms.get.numberOfCells; i++)
    //{
    //  mqttclient.publish(topicBuilder(buff, "Pack_Cells_Voltage/Cell_", itoa((i + 1), msgBuffer, 10)), dtostrf(bms.get.cellVmV[i] / 1000, 5, 3, msgBuffer));
     // mqttclient.publish(topicBuilder(buff, "Pack_Cells_Balance/Cell_", itoa((i + 1), msgBuffer, 10)), bms.get.cellBalanceState[i] ? "true" : "false");
   //}
   // for (size_t i = 0; i < bms.get.numOfTempSensors; i++)
   // {
     // mqttclient.publish(topicBuilder(buff, "Pack_Cell_Temperature_", itoa((i + 1), msgBuffer, 10)), itoa(bms.get.cellTemperature[i], msgBuffer, 10));
   // }
   // mqttclient.publish(topicBuilder(buff, "RelaisOutput_Active"), relaisComparsionResult ? "true" : "false");
   // mqttclient.publish(topicBuilder(buff, "RelaisOutput_Manual"), (_settings.data.relaisFunction == 4) ? "true" : "false"); // should we keep this? you can check with iobroker etc. if you can even switch the relais using mqtt
  }
  else
  {
    char data[JSON_BUFFER];
    serializeJson(Json, data);
    mqttclient.setBufferSize(JSON_BUFFER + 100);
    mqttclient.publish(topicBuilder(buff, "Data"), data, false);
  }
  DEBUG_PRINTLN(F("Done"));
  firstPublish = true;

  return true;
  /*
    if (millis() < (mqtttimer + (_settings._mqttRefresh * 1000)) || _settings._mqttRefresh == 0) // its save for rollover?
    {
      return false;
    }
    mqtttimer = millis();
    if (!mqttclient.connected())
    {
      // delete the esp name string
      if (mqttclient.connect((String(_settings._deviceName)).c_str(), _settings._mqttUser.c_str(), _settings._mqttPassword.c_str()))
      {
  #ifdef SERIALDEBUG
        DEBUG_PRINTLN(F("Reconnected to MQTT SERVER"));
  #endif
        mqttclient.publish((topic + String("/IP")).c_str(), String(WiFi.localIP().toString()).c_str());
      }
      else
      {
  #ifdef SERIALDEBUG
        DEBUG_PRINTLN(F("CANT CONNECT TO MQTT"));
  #endif
        return false; // Exit if we couldnt connect to MQTT brooker
      }
    }
  #ifdef SERIALDEBUG
    DEBUG_PRINTLN(F("Data sent to MQTT Server"));
  #endif

      //qpigs
      mqttclient.publish((String(topic) + String("/Grid_Voltage")).c_str(), String(mppClient.get.variableData.gridVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Grid_Frequenz")).c_str(), String(mppClient.get.variableData.gridFrequency).c_str());
      mqttclient.publish((String(topic) + String("/AC_out_Voltage")).c_str(), String(mppClient.get.variableData.acOutputVoltage).c_str());
      mqttclient.publish((String(topic) + String("/AC_out_Frequenz")).c_str(), String(mppClient.get.variableData.acOutputFrequency).c_str());
      mqttclient.publish((String(topic) + String("/AC_out_VA")).c_str(), String(mppClient.get.variableData.acOutputApparentPower).c_str());
      mqttclient.publish((String(topic) + String("/AC_out_Watt")).c_str(), String(mppClient.get.variableData.acOutputActivePower).c_str());
      mqttclient.publish((String(topic) + String("/AC_out_percent")).c_str(), String(mppClient.get.variableData.outputLoadPercent).c_str());
      mqttclient.publish((String(topic) + String("/Bus_Volt")).c_str(), String(mppClient.get.variableData.busVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Bus_Temp")).c_str(), String(mppClient.get.variableData.inverterHeatSinkTemperature).c_str());
      mqttclient.publish((String(topic) + String("/Battery_Voltage")).c_str(), String(mppClient.get.variableData.batteryVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Battery_Percent")).c_str(), String(mppClient.get.variableData.batteryCapacity).c_str());
      mqttclient.publish((String(topic) + String("/Battery_Charge_A")).c_str(), String(mppClient.get.variableData.batteryChargingCurrent).c_str());
      mqttclient.publish((String(topic) + String("/Battery_Discharge_A")).c_str(), String(mppClient.get.variableData.batteryDischargeCurrent).c_str());
      mqttclient.publish((String(topic) + String("/Battery_SCC_Volt")).c_str(), String(mppClient.get.variableData.batteryVoltageFromScc).c_str());
      mqttclient.publish((String(topic) + String("/PV_Volt")).c_str(), String(mppClient.get.variableData.pvInputVoltage[0]).c_str());
      mqttclient.publish((String(topic) + String("/PV_A")).c_str(), String(mppClient.get.variableData.pvInputCurrent[0]).c_str());
      mqttclient.publish((String(topic) + String("/PV_Watt")).c_str(), String(mppClient.get.variableData.pvChargingPower).c_str());
      //qmod
      mqttclient.publish((String(topic) + String("/Inverter_Operation_Mode")).c_str(), String(mppClient.get.variableData.operationMode).c_str());
      //piri
      mqttclient.publish((String(topic) + String("/Device_Data/Grid_rating_voltage")).c_str(), String(mppClient.get.staticData.gridRatingVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Grid_rating_current")).c_str(), String(mppClient.get.staticData.gridRatingCurrent).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/AC_output_rating_voltage")).c_str(), String(mppClient.get.staticData.acOutputRatingVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/AC_output_rating_frequency")).c_str(), String(mppClient.get.staticData.acOutputRatingFrquency).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/AC_output_rating_current")).c_str(), String(mppClient.get.staticData.acoutputRatingCurrent).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/AC_output_rating_apparent_power")).c_str(), String(mppClient.get.staticData.acOutputRatingApparentPower).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/AC_output_rating_active_power")).c_str(), String(mppClient.get.staticData.acOutputRatingActivePower).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Battery_rating_voltage")).c_str(), String(mppClient.get.staticData.batteryRatingVoltage).c_str());

      mqttclient.publish((String(topic) + String("/Device_Data/Battery_re-charge_voltage")).c_str(), String(mppClient.get.staticData.batteryReChargeVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Battery_under_voltage")).c_str(), String(mppClient.get.staticData.batteryUnderVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Battery_bulk_voltage")).c_str(), String(mppClient.get.staticData.batteryBulkVoltage).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Battery_float_voltage")).c_str(), String(mppClient.get.staticData.batteryFloatVoltage).c_str());

      mqttclient.publish((String(topic) + String("/Device_Data/Battery_type")).c_str(), String(mppClient.get.staticData.batterytype).c_str());

      mqttclient.publish((String(topic) + String("/Device_Data/Current_max_AC_charging_current")).c_str(), String(mppClient.get.staticData.currentMaxAcChargingCurrent).c_str());
      mqttclient.publish((String(topic) + String("/Device_Data/Current_max_charging_current")).c_str(), String(mppClient.get.staticData.currentMaxChargingCurrent).c_str());

    //for test
      //mqttclient.publish((String(topic) + String("/PV_Watt1")).c_str(), String(_qpigsMessage.solarW1).c_str());


    //RAW Messages from Inverter
    #ifdef MQTTDEBUG
      mqttclient.publish((String(topic) + String("/RAW/QPIGS")).c_str(), String(mppClient.get.raw.qpigs).c_str());
      mqttclient.publish((String(topic) + String("/RAW/QPIRI")).c_str(), String(mppClient.get.raw.qpiri).c_str());
      mqttclient.publish((String(topic) + String("/RAW/QMOD")).c_str(), String(mppClient.get.raw.qmod).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QPIWS")).c_str(), String(_qRaw.QPIWS).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QFLAG")).c_str(), String(_qRaw.QFLAG).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QID")).c_str(), String(_qRaw.QID).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QPI")).c_str(), String(_qRaw.QPI).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QMUCHGCR")).c_str(), String(_qRaw.QMUCHGCR).c_str());
      //mqttclient.publish((String(topic) + String("/RAW/QMCHGCR")).c_str(), String(_qRaw.QMCHGCR).c_str());
    #endif
      if(!publishFirst){
       // mqttclient.publish((String(topic) + String("/Device_Control/Set_Command")).c_str(), "NAK");
       // mqttclient.publish((String(topic) + String("/Device_Control/AC_Max_Charge_Current")).c_str(), 000);
       // mqttclient.publish((String(topic) + String("/Device_Control/Max_Charge_Current")).c_str(), 000);
      }
    publishFirst = true;
    return true;
    */
}

void mqttcallback(char *top, unsigned char *payload, unsigned int length)
{
  char buff[256];
  if (!publishFirst)
    return;
  String messageTemp;
  for (unsigned int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }
  if (messageTemp == "NAK" || messageTemp == "(NAK" || messageTemp == "")
    return;
  /*
  //modify the max charging current
  if (strcmp(top, (topic + "/Device_Control/Max_Charge_Current").c_str()) == 0)
  {
    DEBUG_PRINTLN("message recived");
    if (messageTemp.toInt() != _qpiriMessage.battMaxChrgA)
    {
      sendMNCHGC(messageTemp.toInt());
      valChange = true;
    }
  }
  //modify the max ac charging current
  if (strcmp(top, (topic + "/Device_Control/AC_Max_Charge_Current").c_str()) == 0)
  {
    DEBUG_PRINTLN("message recived");
    if (messageTemp.toInt() != _qpiriMessage.battMaxAcChrgA)
    {
      sendMUCHGC(messageTemp.toInt());
      valChange = true;
    }
  }
  */
  // send raw control command
  if (strcmp(top, topicBuilder(buff, "/Device_Control/Set_Command")) == 0)
  {
    DEBUG_PRINTLN("Send Command message recived: " + messageTemp);
    commandFromMqtt = messageTemp;
    // mqttclient.publish((String(topic) + String("/Device_Control/Set_Command_answer")).c_str(), customResponse.c_str());
    valChange = true;
  }
}