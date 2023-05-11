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

#include "webpages/htmlCase.h"      //The HTML Konstructor
#include "webpages/main.h"          //landing page with menu
#include "webpages/settings.h"      //settings page
#include "webpages/settingsedit.h"  //mqtt settings page
#include "webpages/reboot.h"        // Reboot Page
#include "webpages/htmlProzessor.h" // The html Prozessor

#include "PI_Serial/PI_Serial.h"

PI_Serial mppClient(INVERTER_RX, INVERTER_TX);

WiFiClient client;
PubSubClient mqttclient(client);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;
DNSServer dns;
Settings settings;

#include "status-LED.h"

// new importetd
char mqttClientId[80];
ADC_MODE(ADC_VCC);

// flag for saving data
unsigned long mqtttimer = 0;
unsigned long requestTimer = 0;
unsigned long RestartTimer = 0;
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

StaticJsonDocument<JSON_BUFFER> Json;                          // main Json
JsonObject deviceJson = Json.createNestedObject("Device");     // basic device data
JsonObject staticData = Json.createNestedObject("DeviceData"); // battery package data
JsonObject liveData = Json.createNestedObject("liveData");     // battery package data
//----------------------------------------------------------------------
void saveConfigCallback()
{
  DEBUG_PRINTLN(F("Should save config"));
  DEBUG_WEBLN(F("Should save config"));
  shouldSaveConfig = true;
}

void notifyClients()
{
  if (wsClient != nullptr && wsClient->canSend())
  {
    DEBUG_PRINT(F("Info: Data sent to WebSocket... "));
    DEBUG_WEB(F("Info: Data sent to WebSocket... "));
    char data[JSON_BUFFER];
    size_t len = serializeJson(Json, data);
    wsClient->text(data, len);
    DEBUG_PRINTLN(F("Done"));
    DEBUG_WEBLN(F("Done"));
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    // updateProgress = true;
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsClient = client;
    // getJsonDevice();
    getJsonData();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    wsClient = nullptr;
    break;
  case WS_EVT_DATA:
    // bmstimer = millis();
    mqtttimer = millis();
    handleWebSocketMessage(arg, data, len);
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
    DEBUG_WEBLN("Update");
    Update.runAsync(true);
    if (!Update.begin(free_space))
    {
      Update.printError(Serial);
      ESP.restart();
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
    ESP.restart();
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
      ESP.restart();
    }
    else
    {

      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_REBOOT, htmlProcessor);
      request->send(response);
      restartNow = true; // Set flag so main loop can issue restart call
      RestartTimer = millis();
      DEBUG_PRINTLN("Update complete");
      DEBUG_WEBLN("Update complete");
    }
  }
}

#ifdef isDEBUG
/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (uint i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
}
#endif

void setup()
{
#ifdef DEBUG
  DEBUG_BEGIN(9600); // Debugging towards UART1
#endif
  settings.load();
  WiFi.persistent(true); // fix wifi save bug
  WiFi.hostname(settings.data.deviceName);
  AsyncWiFiManager wm(&server, &dns);
  sprintf(mqttClientId, "%s-%06X", settings.data.deviceName, ESP.getChipId());

  DEBUG_PRINTLN();
  DEBUG_PRINTF("Device Name:\t");
  DEBUG_PRINTLN(settings.data.deviceName);
  DEBUG_PRINTF("Mqtt Server:\t");
  DEBUG_PRINTLN(settings.data.mqttServer);
  DEBUG_PRINTF("Mqtt Port:\t");
  DEBUG_PRINTLN(settings.data.mqttPort);
  DEBUG_PRINTF("Mqtt User:\t");
  DEBUG_PRINTLN(settings.data.mqttUser);
  DEBUG_PRINTF("Mqtt Passwort:\t");
  DEBUG_PRINTLN(settings.data.mqttPassword);
  DEBUG_PRINTF("Mqtt Interval:\t");
  DEBUG_PRINTLN(settings.data.mqttRefresh);
  DEBUG_PRINTF("Mqtt Topic:\t");
  DEBUG_PRINTLN(settings.data.mqttTopic);
  DEBUG_WEBLN();
  DEBUG_WEBF("Device Name:\t");
  DEBUG_WEBLN(settings.data.deviceName);
  DEBUG_WEBF("Mqtt Server:\t");
  DEBUG_WEBLN(settings.data.mqttServer);
  DEBUG_WEBF("Mqtt Port:\t");
  DEBUG_WEBLN(settings.data.mqttPort);
  DEBUG_WEBF("Mqtt User:\t");
  DEBUG_WEBLN(settings.data.mqttUser);
  DEBUG_WEBF("Mqtt Passwort:\t");
  DEBUG_WEBLN(settings.data.mqttPassword);
  DEBUG_WEBF("Mqtt Interval:\t");
  DEBUG_WEBLN(settings.data.mqttRefresh);
  DEBUG_WEBF("Mqtt Topic:\t");
  DEBUG_WEBLN(settings.data.mqttTopic);

  mppClient.setProtocol(100); // manual set the protocol
  mppClient.Init();           // init the PI_serial Library

  // create custom wifimanager fields

  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 100);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", "Solar01", 30);
  AsyncWiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", "1883", 6);
  AsyncWiFiManagerParameter custom_mqtt_refresh("mqtt_refresh", "MQTT Send Interval", "300", 4);
  AsyncWiFiManagerParameter custom_device_name("device_name", "Device Name", "Solar2MQTT", 40);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_refresh);
  wm.addParameter(&custom_device_name);

  bool apRunning = wm.autoConnect("Solar2MQTT-AP");
  wm.setDebugOutput(false);       // disable wifimanager debug output
  wm.setMinimumSignalQuality(20); // filter weak wifi signals
  wm.setConnectTimeout(15);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wm.setSaveConfigCallback(saveConfigCallback);

  // save settings if wifi setup is fire up
  if (shouldSaveConfig)
  {
    strncpy(settings.data.mqttServer, custom_mqtt_server.getValue(), 40);
    strncpy(settings.data.mqttUser, custom_mqtt_user.getValue(), 40);
    strncpy(settings.data.mqttPassword, custom_mqtt_pass.getValue(), 40);
    settings.data.mqttPort = atoi(custom_mqtt_port.getValue());
    strncpy(settings.data.deviceName, custom_device_name.getValue(), 40);
    strncpy(settings.data.mqttTopic, custom_mqtt_topic.getValue(), 40);
    settings.data.mqttRefresh = atoi(custom_mqtt_refresh.getValue());
    settings.save();
    settings.load();
    //ESP.restart();
  }

  mqttclient.setServer(settings.data.mqttServer, settings.data.mqttPort);
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
              AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_MAIN, htmlProcessor);
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

                liveJson["device_name"] = settings.data.deviceName;
                serializeJson(liveJson, *response);
                request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_REBOOT, htmlProcessor);
                request->send(response);
                restartNow = true;
                RestartTimer = millis(); });

    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_CONFIRM_RESET, htmlProcessor);
                request->send(response); });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Device is Erasing...");
                response->addHeader("Refresh", "15; url=/");
                response->addHeader("Connection", "close");
                request->send(response);
                delay(1000);
                settings.reset();
                ESP.eraseConfig();
                ESP.restart(); });

    server.on("/settingsedit", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS_EDIT, htmlProcessor);
                request->send(response); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS, htmlProcessor);
                request->send(response); });
/*
    server.on("/settingsjson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                DynamicJsonDocument SettingsJson(256);
                SettingsJson["device_name"] = settings.data.deviceName;
                SettingsJson["mqtt_server"] = settings.data.mqttServer;
                SettingsJson["mqtt_port"] = settings.data.mqttPort;
                SettingsJson["mqtt_topic"] = settings.data.mqttTopic;
                SettingsJson["mqtt_user"] = settings.data.mqttUser;
                SettingsJson["mqtt_password"] = settings.data.mqttPassword;
                SettingsJson["mqtt_refresh"] = settings.data.mqttRefresh;
                serializeJson(SettingsJson, *response);
                request->send(response); });
*/
    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                strncpy(settings.data.mqttServer, request->arg("post_mqttServer").c_str(), 40);
                settings.data.mqttPort = request->arg("post_mqttPort").toInt();
                strncpy(settings.data.mqttUser, request->arg("post_mqttUser").c_str(), 40);
                strncpy(settings.data.mqttPassword, request->arg("post_mqttPassword").c_str(), 40);
                strncpy(settings.data.mqttTopic, request->arg("post_mqttTopic").c_str(), 40);
                settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prevent lower numbers
                strncpy(settings.data.deviceName, request->arg("post_deviceName").c_str(), 40);
                settings.save();
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
          Serial.end();
          ws.enable(false);
          ws.closeAll(); },
        handle_update_progress_cb);

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(418, "text/plain", "418 I'm a teapot"); });

    DEBUG_PRINTLN("Webserver Running...");
    DEBUG_WEBLN("Webserver Running...");

    // set the device name
    MDNS.addService("http", "tcp", 80);
    if (MDNS.begin(settings.data.deviceName))
      DEBUG_PRINTLN(F("mDNS running..."));
    DEBUG_WEBLN(F("mDNS running..."));
    ws.onEvent(onEvent);
    server.addHandler(&ws);
#ifdef isDEBUG
    // WebSerial is accessible at "<IP Address>/webserial" in browser
    WebSerial.begin(&server);
    /* Attach Message Callback */
    WebSerial.onMessage(recvMsg);
#endif
    server.begin();
  }
}

void loop()
{

  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  {                      // No use going to next step unless WIFI is up and running.
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();

    if (valChange)
    {
      if (commandFromWeb != "")
      {
        DEBUG_PRINTLN(commandFromWeb);
        DEBUG_WEBLN(commandFromWeb);
        String tmp = mppClient.sendCommand(commandFromWeb); // send a custom command to the device
        DEBUG_PRINTLN(tmp);
        DEBUG_WEBLN(tmp);
        commandFromWeb = "";
      }
      if (commandFromMqtt != "")
      {
        DEBUG_PRINTLN(commandFromMqtt);
        DEBUG_WEBLN(commandFromMqtt);
        String customResponse = mppClient.sendCommand(commandFromMqtt); // send a custom command to the device
        DEBUG_PRINTLN(customResponse);
        DEBUG_WEBLN(customResponse);
        commandFromMqtt = "";
        mqttclient.publish((String(settings.data.mqttTopic) + String("/Device_Control/Set_Command_answer")).c_str(), (customResponse).c_str());
      }
      mppClient.getStaticeData();
      mqtttimer = 0;
      requestTimer = 0;
      valChange = false;
    }

    mppClient.getVariableData();

    if (millis() >= (requestTimer + (3 * 1000)) /*&& wsClient != nullptr && wsClient->canSend()*/)
    {
      // mppClient.getVariableData();
      if (!askInverterOnce)
      {
        mppClient.getStaticeData();
        askInverterOnce = true;
      }
      requestTimer = millis();
    }
    if (millis() >= (mqtttimer + (settings.data.mqttRefresh * 1000)))
    {
      sendtoMQTT(); // Update data to MQTT server if we should
                    //        getJsonDevice();
      getJsonData();
      notifyClients();

      mqtttimer = millis();
    }
    notificationLED(); // notification LED routine
    mqttclient.loop(); // Check if we have something to read from MQTT
  }

  if (restartNow && millis() >= (RestartTimer + 500))
  {
    DEBUG_PRINTLN("Restart");
    DEBUG_WEBLN("Restart");
    ESP.restart();
  }
}

void getJsonData()
{
  staticData["gridV"] = mppClient.get.variableData.gridVoltage;
  staticData["gridHz"] = mppClient.get.variableData.gridFrequency;
  staticData["acOutV"] = mppClient.get.variableData.acOutputVoltage;
  staticData["acOutHz"] = mppClient.get.variableData.acOutputFrequency;
  staticData["acOutVa"] = mppClient.get.variableData.acOutputApparentPower;
  staticData["acOutW"] = mppClient.get.variableData.acOutputActivePower;
  staticData["acOutPercent"] = mppClient.get.variableData.outputLoadPercent;
  staticData["busV"] = mppClient.get.variableData.busVoltage;
  staticData["heatSinkDegC"] = mppClient.get.variableData.inverterHeatSinkTemperature;
  staticData["battV"] = mppClient.get.variableData.batteryVoltage;
  staticData["battPercent"] = mppClient.get.variableData.batteryCapacity;
  staticData["battChargeA"] = mppClient.get.variableData.batteryChargingCurrent;
  staticData["battDischargeA"] = mppClient.get.variableData.batteryDischargeCurrent;
  staticData["sccBattV"] = mppClient.get.variableData.batteryVoltageFromScc;
  staticData["solarV"] = mppClient.get.variableData.pvInputVoltage[0];
  staticData["solarA"] = mppClient.get.variableData.pvInputCurrent[0];
  staticData["solarW"] = mppClient.get.variableData.pvChargingPower; // not realy?
  staticData["iv_mode"] = mppClient.get.variableData.operationMode;
  staticData["device_name"] = settings.data.deviceName;
}

char *topicBuilder(char *buffer, char const *path, char const *numering = "")
{                                                  // buffer, topic
  const char *mainTopic = settings.data.mqttTopic; // get the main topic path
  strcpy(buffer, mainTopic);
  strcat(buffer, "/");
  strcat(buffer, path);
  strcat(buffer, numering);
  return buffer;
}

bool connectMQTT()
{
  if (strcmp(settings.data.mqttServer, "") == 0)
    return false;
  char buff[256];
  if (!mqttclient.connected())
  {
    firstPublish = false;
    DEBUG_PRINT(F("Info: MQTT Client State is: "));
    DEBUG_PRINTLN(mqttclient.state());
    DEBUG_PRINT(F("Info: establish MQTT Connection... "));
    DEBUG_WEB(F("Info: MQTT Client State is: "));
    DEBUG_WEBLN(mqttclient.state());
    DEBUG_WEB(F("Info: establish MQTT Connection... "));

    if (mqttclient.connect(mqttClientId, settings.data.mqttUser, settings.data.mqttPassword, (topicBuilder(buff, "alive")), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {
        DEBUG_PRINTLN(F("Done"));
        DEBUG_WEBLN(F("Done"));
        mqttclient.publish(topicBuilder(buff, "alive"), "true", true); // LWT online message must be retained!
        mqttclient.publish(topicBuilder(buff, "Device_IP"), (const char *)(WiFi.localIP().toString()).c_str(), true);
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Set_Command"));

        if (settings.data.relaisFunction == 4)
          mqttclient.subscribe(topicBuilder(buff, "Device_Control/Relais"));
      }
      else
      {
        DEBUG_PRINT(F("Fail\n"));
        DEBUG_WEB(F("Fail\n"));
      }
    }
    else
    {
      DEBUG_PRINT(F("Fail\n"));
      DEBUG_WEB(F("Fail\n"));
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
    DEBUG_WEBLN(F("Error: No connection to MQTT Server, cant send Data!"));
    firstPublish = false;
    return false;
  }
  // DEBUG_PRINT(F("Info: Data sent to MQTT Server... "));
  DEBUG_PRINTLN(F("Info: Data sent to MQTT Server... "));
  DEBUG_WEBLN(F("Info: Data sent to MQTT Server... "));

  if (!settings.data.mqttJson)
  {
    // QPIGS
    if (mppClient.get.variableData.gridVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Grid_Voltage"), dtostrf(mppClient.get.variableData.gridVoltage, 5, 1, msgBuffer));
    if (mppClient.get.variableData.gridFrequency != -1)
      mqttclient.publish(topicBuilder(buff, "Grid_Frequenz"), dtostrf(mppClient.get.variableData.gridFrequency, 4, 1, msgBuffer));
    if (mppClient.get.variableData.acOutputVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "AC_out_Voltage"), dtostrf(mppClient.get.variableData.acOutputVoltage, 5, 1, msgBuffer));
    if (mppClient.get.variableData.acOutputFrequency != -1)
      mqttclient.publish(topicBuilder(buff, "AC_out_Frequenz"), dtostrf(mppClient.get.variableData.acOutputFrequency, 4, 1, msgBuffer));
    if (mppClient.get.variableData.acOutputApparentPower != -1)
      mqttclient.publish(topicBuilder(buff, "AC_out_VA"), itoa(mppClient.get.variableData.acOutputApparentPower, msgBuffer, 10));
    if (mppClient.get.variableData.acOutputActivePower != -1)
      mqttclient.publish(topicBuilder(buff, "AC_out_Watt"), itoa(mppClient.get.variableData.acOutputActivePower, msgBuffer, 10));
    if (mppClient.get.variableData.outputLoadPercent != -1)
      mqttclient.publish(topicBuilder(buff, "AC_out_percent"), itoa(mppClient.get.variableData.outputLoadPercent, msgBuffer, 10));
    if (mppClient.get.variableData.busVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Bus_Volt"), itoa(mppClient.get.variableData.busVoltage, msgBuffer, 10));
    if (mppClient.get.variableData.inverterHeatSinkTemperature != -1)
      mqttclient.publish(topicBuilder(buff, "Bus_Temp"), itoa(mppClient.get.variableData.inverterHeatSinkTemperature, msgBuffer, 10));
    if (mppClient.get.variableData.batteryVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Battery_Voltage"), dtostrf(mppClient.get.variableData.batteryVoltage, 4, 1, msgBuffer));
    if (mppClient.get.variableData.batteryCapacity != -1)
      mqttclient.publish(topicBuilder(buff, "Battery_Percent"), itoa(mppClient.get.variableData.batteryCapacity, msgBuffer, 10));
    if (mppClient.get.variableData.batteryChargingCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Battery_Charge_A"), itoa(mppClient.get.variableData.batteryChargingCurrent, msgBuffer, 10));
    if (mppClient.get.variableData.batteryDischargeCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Battery_Discharge_A"), itoa(mppClient.get.variableData.batteryDischargeCurrent, msgBuffer, 10));
    if (mppClient.get.variableData.batteryVoltageFromScc != -1)
      mqttclient.publish(topicBuilder(buff, "Battery_SCC_Volt"), dtostrf(mppClient.get.variableData.batteryVoltageFromScc, 4, 1, msgBuffer));

    if (mppClient.get.variableData.pvInputVoltage[1] != -1)
    {
      for (size_t i : mppClient.get.variableData.pvInputVoltage)
      {
        if (mppClient.get.variableData.pvInputVoltage[i] != -1)
          mqttclient.publish(topicBuilder(buff, "PV_Volt_" + i), String(mppClient.get.variableData.pvInputVoltage[i]).c_str());
      }
    }
    else if (mppClient.get.variableData.pvInputVoltage[0] != -1)
    {
      mqttclient.publish(topicBuilder(buff, "PV_Volt"), String(mppClient.get.variableData.pvInputVoltage[0]).c_str());
    }

    if (mppClient.get.variableData.pvInputCurrent[1] != -1)
    {
      for (size_t i : mppClient.get.variableData.pvInputCurrent)
      {
        if (mppClient.get.variableData.pvInputCurrent[i] != -1)
          mqttclient.publish(topicBuilder(buff, "PV_A_" + i), String(mppClient.get.variableData.pvInputCurrent[i]).c_str());
      }
    }
    else if (mppClient.get.variableData.pvInputCurrent[0] != -1)
    {
      if (mppClient.get.variableData.pvInputCurrent[0] != -1)
        mqttclient.publish(topicBuilder(buff, "PV_A"), String(mppClient.get.variableData.pvInputCurrent[0]).c_str());
    }

    if (mppClient.get.variableData.pvChargingPower != -1)
      mqttclient.publish(topicBuilder(buff, "PV_Watt"), String(mppClient.get.variableData.pvChargingPower).c_str());

    // QMOD
    if (strcmp(mppClient.get.variableData.operationMode, "") != 0)
      mqttclient.publish(topicBuilder(buff, "Inverter_Operation_Mode"), mppClient.get.variableData.operationMode);

    // QALL
    if (mppClient.get.variableData.pvGenerationDay != -1)
      mqttclient.publish(topicBuilder(buff, "PV_generation_day"), itoa(mppClient.get.variableData.pvGenerationDay, msgBuffer, 10));
    if (mppClient.get.variableData.pvGenerationSum != -1)
      mqttclient.publish(topicBuilder(buff, "PV_generation_sum"), itoa(mppClient.get.variableData.pvGenerationSum, msgBuffer, 10));

    // QPIRI
    if (mppClient.get.staticData.gridRatingVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Grid_rating_voltage"), dtostrf(mppClient.get.staticData.gridRatingVoltage, 5, 1, msgBuffer));
    if (mppClient.get.staticData.gridRatingCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Grid_rating_current"), dtostrf(mppClient.get.staticData.gridRatingCurrent, 4, 1, msgBuffer));
    if (mppClient.get.staticData.acOutputRatingVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_output_rating_voltage"), dtostrf(mppClient.get.staticData.acOutputRatingVoltage, 5, 1, msgBuffer));
    if (mppClient.get.staticData.acOutputRatingFrquency != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_output_rating_frequency"), dtostrf(mppClient.get.staticData.acOutputRatingFrquency, 4, 1, msgBuffer));
    if (mppClient.get.staticData.acoutputRatingCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_output_rating_current"), dtostrf(mppClient.get.staticData.acoutputRatingCurrent, 4, 1, msgBuffer));
    if (mppClient.get.staticData.acOutputRatingApparentPower != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_output_rating_apparent_power"), itoa(mppClient.get.staticData.acOutputRatingApparentPower, msgBuffer, 10));
    if (mppClient.get.staticData.acOutputRatingActivePower != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_output_rating_active_power"), itoa(mppClient.get.staticData.acOutputRatingActivePower, msgBuffer, 10));
    if (mppClient.get.staticData.batteryRatingVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_rating_voltage"), dtostrf(mppClient.get.staticData.batteryRatingVoltage, 4, 1, msgBuffer));
    if (mppClient.get.staticData.batteryReChargeVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_re-charge_voltage"), dtostrf(mppClient.get.staticData.batteryReChargeVoltage, 4, 1, msgBuffer));
    if (mppClient.get.staticData.batteryUnderVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_under_voltage"), dtostrf(mppClient.get.staticData.batteryUnderVoltage, 4, 1, msgBuffer));
    if (mppClient.get.staticData.batteryBulkVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_bulk_voltage"), dtostrf(mppClient.get.staticData.batteryBulkVoltage, 4, 1, msgBuffer));
    if (mppClient.get.staticData.batteryFloatVoltage != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_float_voltage"), dtostrf(mppClient.get.staticData.batteryFloatVoltage, 4, 1, msgBuffer));
    if (strcmp(mppClient.get.staticData.batterytype, "") != 0)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_type"), mppClient.get.staticData.batterytype);
    if (mppClient.get.staticData.currentMaxAcChargingCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Current_max_AC_charging_current"), itoa(mppClient.get.staticData.currentMaxAcChargingCurrent, msgBuffer, 10));
    if (mppClient.get.staticData.currentMaxChargingCurrent != -1)
      mqttclient.publish(topicBuilder(buff, "Device_Data/Current_max_charging_current"), itoa(mppClient.get.staticData.currentMaxChargingCurrent, msgBuffer, 10));

// RAW
#ifdef DEBUG
    if (mppClient.get.raw.qpigs != "")
      mqttclient.publish(topicBuilder(buff, "RAW/QPIGS"), (mppClient.get.raw.qpigs).c_str());
    if (mppClient.get.raw.qpiri != "")
      mqttclient.publish(topicBuilder(buff, "RAW/QPIRI"), (mppClient.get.raw.qpiri).c_str());
    if (mppClient.get.raw.qmod != "")
      mqttclient.publish(topicBuilder(buff, "RAW/QMOD"), (mppClient.get.raw.qmod).c_str());
    if (mppClient.get.raw.qall != "")
      mqttclient.publish(topicBuilder(buff, "RAW/QALL"), (mppClient.get.raw.qall).c_str());
#endif
  }
  else
  {
    char data[JSON_BUFFER];
    serializeJson(Json, data);
    mqttclient.setBufferSize(JSON_BUFFER + 100);
    mqttclient.publish(topicBuilder(buff, "Data"), data, false);
  }
  DEBUG_PRINTLN(F("Done"));
  DEBUG_WEBLN(F("Done"));
  firstPublish = true;

  return true;
}

void mqttcallback(char *top, unsigned char *payload, unsigned int length)
{
  char buff[256];
  // if (!publishFirst)
  //   return;
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
  if (strcmp(top, topicBuilder(buff, "Device_Control/Set_Command")) == 0)
  {
    DEBUG_PRINT(F("Send Command message recived: "));
    DEBUG_PRINTLN(messageTemp);
    DEBUG_WEB(F("Send Command message recived: "));
    DEBUG_WEBLN(messageTemp);

    commandFromMqtt = messageTemp;
    mqttclient.publish(topicBuilder(buff, "Device_Control/Set_Command_answer"), customResponse.c_str());
    valChange = true;
  }
}

/* later
void DEBUG_PRINT(const __FlashStringHelper *logmessage)
{
  DEBUG_PRINT(logmessage);
  DEBUG_WEB(logmessage);
}
void DEBUG_PRINTLN(const __FlashStringHelper *logmessage)
{
  DEBUG_PRINTLN(logmessage);
  DEBUG_WEBLN(logmessage);
}
*/
