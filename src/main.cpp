
/*
Solar2MQTT Project
https://github.com/softwarecrash/Solar2MQTT
*/

#include "main.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Settings.h"

#include "html.h"
#include "htmlProzessor.h"

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
long mqtttimer = 0;
unsigned long requestTimer = 0;
unsigned long RestartTimer = 0;
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool valChange = false;
bool askInverterOnce = false;
bool fwUpdateRunning = false;
bool publishFirst = false;
String commandFromWeb;
String commandFromMqtt;
String customResponse;

bool firstPublish;
DynamicJsonDocument Json(JSON_BUFFER);                         // main Json
JsonObject ivJson = Json.createNestedObject("Device");         // basic device data
JsonObject staticData = Json.createNestedObject("DeviceData"); // battery package data
JsonObject liveData = Json.createNestedObject("LiveData");     // battery package data
JsonObject rawData = Json.createNestedObject("RawData");       // battery package data

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
    DEBUG_PRINT(F("Data sent to WebSocket... "));
    DEBUG_WEB(F("Data sent to WebSocket... "));
    size_t len = measureJson(Json);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
    if (buffer)
    {
      serializeJson(Json, (char *)buffer->get(), len + 1);
      wsClient->text(buffer);
    }
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
    ws.cleanupClients(); // clean unused client connections
    break;
  case WS_EVT_DATA:
    // bmstimer = millis();
    mqtttimer = millis();
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    wsClient = nullptr;
    ws.cleanupClients(); // clean unused client connections
    break;
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
  DEBUG_BEGIN(DEBUG_BAUD); // Debugging towards UART1
#endif
  settings.load();
  WiFi.persistent(true); // fix wifi save bug
  WiFi.hostname(settings.data.deviceName);
  AsyncWiFiManager wm(&server, &dns);
  sprintf(mqttClientId, "%s-%06X", settings.data.deviceName, ESP.getChipId());

#ifdef DEBUG
  wm.setDebugOutput(true); // enable wifimanager debug output
#else
  wm.setDebugOutput(false); // disable wifimanager debug output
#endif
  wm.setMinimumSignalQuality(20); // filter weak wifi signals
  wm.setConnectTimeout(15);       // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wm.setSaveConfigCallback(saveConfigCallback);

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

  // create custom wifimanager fields

  AsyncWiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Password", NULL, 40);
  AsyncWiFiManagerParameter custom_mqtt_topic("mqtt_topic", "MQTT Topic", "Solar01", 40);
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
  /*
    #ifdef DEBUG
      wm.setDebugOutput(true);       // enable wifimanager debug output
    #else
      wm.setDebugOutput(false);       // disable wifimanager debug output
    #endif
    wm.setMinimumSignalQuality(20); // filter weak wifi signals
    wm.setConnectTimeout(15);       // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
    wm.setSaveConfigCallback(saveConfigCallback);
  */
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
    ESP.restart();
  }

  mqttclient.setServer(settings.data.mqttServer, settings.data.mqttPort);
  DEBUG_PRINTLN(F("MQTT Server config Loaded"));
  DEBUG_WEBLN(F("MQTT Server config Loaded"));

  mqttclient.setCallback(mqttcallback);
  // mqttclient.setBufferSize(MQTT_BUFFER);

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
                serializeJson(Json, *response);
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

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                strncpy(settings.data.mqttServer, request->arg("post_mqttServer").c_str(), 40);
                settings.data.mqttPort = request->arg("post_mqttPort").toInt();
                strncpy(settings.data.mqttUser, request->arg("post_mqttUser").c_str(), 40);
                strncpy(settings.data.mqttPassword, request->arg("post_mqttPassword").c_str(), 40);
                strncpy(settings.data.mqttTopic, request->arg("post_mqttTopic").c_str(), 40);
                settings.data.mqttRefresh = request->arg("post_mqttRefresh").toInt() < 1 ? 1 : request->arg("post_mqttRefresh").toInt(); // prevent lower numbers
                strncpy(settings.data.deviceName, request->arg("post_deviceName").c_str(), 40);
                settings.data.mqttJson = (request->arg("post_mqttjson") == "true") ? true : false;
                strncpy(settings.data.mqttTriggerPath, request->arg("post_mqtttrigger").c_str(), 80);
                settings.data.webUIdarkmode = (request->arg("post_webuicolormode") == "true") ? true : false;
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
    //https://gist.github.com/JMishou/60cb762047b735685e8a09cd2eb42a60
    // the request handler is triggered after the upload has finished... 
    // create the response, add header, and send response
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    //restartNow = true; // Tell the main loop to restart the ESP
    //RestartTimer = millis();  // Tell the main loop to restart the ESP
    request->send(response); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
          // Upload handler chunks in data

          if (!index)
          { // if index == 0 then this is the first frame of data
            Serial.printf("UploadStart: %s\n", filename.c_str());
            Serial.setDebugOutput(true);

            // calculate sketch space required for the update
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace))
            { // start with max available size
              Update.printError(Serial);
            }
            Update.runAsync(true); // tell the updaterClass to run in async mode
          }

          // Write chunked data to the free sketch space
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }

          if (final)
          { // if the final flag is set then this is the last frame of data
            if (Update.end(true))
            { // true to set the size to the current progress
              Serial.printf("Update Success: %u B\nRebooting...\n", index + len);
            }
            else
            {
              Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
          }
        });

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

    mppClient.setProtocol(100); // manual set the protocol
    mppClient.Init();           // init the PI_serial Library
    mppClient.callback(prozessData);

    mqtttimer = (settings.data.mqttRefresh * 1000) * (-1);
  }
}

void loop()
{

  // Make sure wifi is in the right mode
  if (WiFi.status() == WL_CONNECTED)
  {                      // No use going to next step unless WIFI is up and running.
    ws.cleanupClients(); // clean unused client connections
    MDNS.update();
    if (!fwUpdateRunning)
      mppClient.loop(); // Call the PI Serial Library loop
    mqttclient.loop();  // Check if we have something to read from MQTT
    notificationLED();  // notification LED routine
  }

  if (restartNow && millis() >= (RestartTimer + 500))
  {
    DEBUG_PRINTLN("Restart");
    DEBUG_WEBLN("Restart");
    ESP.restart();
  }
}

void prozessData()
{
  DEBUG_PRINTLN("ProzessData called");
  getJsonData();
  if (wsClient != nullptr && wsClient->canSend())
    notifyClients();
  if (millis() - mqtttimer > (settings.data.mqttRefresh * 1000))
  {
    sendtoMQTT(); // Update data to MQTT server if we should
    mqtttimer = millis();
  }

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
      // mqttclient.publish((String(settings.data.mqttTopic) + String("/Device_Control/Set_Command_answer")).c_str(), (customResponse).c_str());
    }
    // mqtttimer = 0;
    mqtttimer = (settings.data.mqttRefresh * 1000) * (-1);
    requestTimer = 0;
    valChange = false;
  }
}

void getJsonData()
{
  // Json["EP_"]["LiveData"]["CONNECTION"] = 123;

  /*
  deviceJson[F("device_name")] = settings.data.deviceName;
  deviceJson[F("ESP_VCC")] = ESP.getVcc() / 1000.0;
  deviceJson[F("Wifi_RSSI")] = WiFi.RSSI();
  deviceJson[F("sw_version")] = SOFTWARE_VERSION;
  deviceJson[F("Flash_Size")] = ESP.getFlashChipSize();
  deviceJson[F("Sketch_Size")] = ESP.getSketchSize();
  deviceJson[F("Free_Sketch_Space")] = ESP.getFreeSketchSpace();
  deviceJson[F("CPU_Frequency")] = ESP.getCpuFreqMHz();
  deviceJson[F("Real_Flash_Size")] = ESP.getFlashChipRealSize();
  deviceJson[F("Free_Heap")] = ESP.getFreeHeap();
  deviceJson[F("HEAP_Fragmentation")] = ESP.getHeapFragmentation();
  deviceJson[F("Free_BlockSize")] = ESP.getMaxFreeBlockSize();

  liveData["gridV"] = mppClient.get.variableData.gridVoltage;
  liveData["gridHz"] = mppClient.get.variableData.gridFrequency;
  liveData["acOutV"] = mppClient.get.variableData.acOutputVoltage;
  liveData["acOutHz"] = mppClient.get.variableData.acOutputFrequency;
  liveData["acOutVa"] = mppClient.get.variableData.acOutputApparentPower;
  liveData["acOutW"] = mppClient.get.variableData.acOutputActivePower;
  liveData["acOutPercent"] = mppClient.get.variableData.outputLoadPercent;
  liveData["busV"] = mppClient.get.variableData.busVoltage;
  liveData["heatSinkDegC"] = mppClient.get.variableData.inverterHeatSinkTemperature;
  liveData["battV"] = mppClient.get.variableData.batteryVoltage;
  liveData["battPercent"] = mppClient.get.variableData.batteryCapacity;
  liveData["battChargeA"] = mppClient.get.variableData.batteryChargingCurrent;
  liveData["battDischargeA"] = mppClient.get.variableData.batteryDischargeCurrent;
  liveData["sccBattV"] = mppClient.get.variableData.batteryVoltageFromScc;
  liveData["solarV"] = mppClient.get.variableData.pvInputVoltage[0];
  liveData["solarA"] = mppClient.get.variableData.pvInputCurrent[0];
  liveData["solarW"] = mppClient.get.variableData.pvChargingPower[0];
  liveData["iv_mode"] = mppClient.get.variableData.operationMode;

  if (mppClient.qAvaible.qpiri)
  {
    staticData["AC_in_rating_voltage"] = mppClient.get.staticData.gridRatingVoltage;
    staticData["AC_in_rating_current"] = mppClient.get.staticData.gridRatingCurrent;
    staticData["AC_out_rating_voltage"] = mppClient.get.staticData.acOutputRatingVoltage;
    staticData["AC_out_rating_frequency"] = mppClient.get.staticData.acOutputRatingFrquency;
    staticData["AC_out_rating_current"] = mppClient.get.staticData.acoutputRatingCurrent;
    staticData["AC_out_rating_apparent_power"] = mppClient.get.staticData.acOutputRatingApparentPower;
    staticData["AC_out_rating_active_power"] = mppClient.get.staticData.acOutputRatingActivePower;
    staticData["Battery_rating_voltage"] = mppClient.get.staticData.batteryRatingVoltage;
    staticData["Battery_re-charge_voltage"] = mppClient.get.staticData.batteryReChargeVoltage;
    staticData["Battery_under_voltage"] = mppClient.get.staticData.batteryUnderVoltage;
    staticData["Battery_bulk_voltage"] = mppClient.get.staticData.batteryBulkVoltage;
    staticData["Battery_float_voltage"] = mppClient.get.staticData.batteryFloatVoltage;

    staticData["Battery_type"] = mppClient.get.staticData.batterytype;
    staticData["Current_max_AC_charging_current"] = mppClient.get.staticData.currentMaxAcChargingCurrent;
    staticData["Current_max_charging_current"] = mppClient.get.staticData.currentMaxChargingCurrent;
  }
  // QPI
  if (mppClient.qAvaible.qpi)
  {
    staticData["Protocol_ID"] = mppClient.get.staticData.deviceProtocol;
  }
  // QMN
  if (mppClient.qAvaible.qmn)
  {
    staticData["Device_Model"] = mppClient.get.staticData.modelName;
  }
  */
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
    DEBUG_PRINT(F("MQTT Client State is: "));
    DEBUG_PRINTLN(mqttclient.state());
    DEBUG_PRINT(F("establish MQTT Connection... "));
    DEBUG_WEB(F("MQTT Client State is: "));
    DEBUG_WEBLN(mqttclient.state());
    DEBUG_WEB(F("establish MQTT Connection... "));

    if (mqttclient.connect(mqttClientId, settings.data.mqttUser, settings.data.mqttPassword, (topicBuilder(buff, "alive")), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {
        DEBUG_PRINTLN(F("Done"));
        DEBUG_WEBLN(F("Done"));
        mqttclient.publish(topicBuilder(buff, "alive"), "true", true); // LWT online message must be retained!
        mqttclient.publish(topicBuilder(buff, "IP"), (const char *)(WiFi.localIP().toString()).c_str(), true);
        mqttclient.subscribe(topicBuilder(buff, "Device_Control/Set_Command"));
        if (strlen(settings.data.mqttTriggerPath) > 0)
        {
          DEBUG_WEBLN("MQTT Data Trigger Subscribed");
          mqttclient.subscribe(settings.data.mqttTriggerPath);
        }
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

char *topicBuilder1(char *buffer, char const *path0, char const *path1)
{                                                  // buffer, topic
  const char *mainTopic = settings.data.mqttTopic; // get the main topic path
  strcpy(buffer, mainTopic);
  strcat(buffer, "/");
  strcat(buffer, path0);
  strcat(buffer, "/");
  strcat(buffer, path1);
  return buffer;
}

bool sendtoMQTT()
{
  char buff[256]; // temp buffer for the topic string
  if (!connectMQTT())
  {
    DEBUG_PRINTLN(F("Error: No connection to MQTT Server, cant send Data!"));
    DEBUG_WEBLN(F("Error: No connection to MQTT Server, cant send Data!"));
    firstPublish = false;
    return false;
  }
  DEBUG_PRINT(F("Data sent to MQTT Server... "));
  DEBUG_WEB(F("Data sent to MQTT Server... "));
  mqttclient.publish(topicBuilder(buff, "alive"), "true", true); // LWT online message must be retained!
  if (!settings.data.mqttJson)
  {

    for (JsonPair jsonDev : Json.as<JsonObject>())
    {
      for (JsonPair jsondat : jsonDev.value().as<JsonObject>())
      {
        char msgBuffer1[200];
        sprintf(msgBuffer1, "%s/%s/%s", settings.data.mqttTopic, jsonDev.key().c_str(), jsondat.key().c_str());
        DEBUG_PRINTLN(msgBuffer1);
        mqttclient.publish(msgBuffer1, jsondat.value().as<String>().c_str());
      }
    }

/*
    // testing
    mqttclient.publish(topicBuilder(buff, "Device_Control/Set_Command_answer"), mppClient.get.raw.commandAnswer.c_str());
    // Q1
    if (mppClient.qAvaible.q1)
    {
      mqttclient.publish(topicBuilder(buff, "Inverter_Tracker_Temperature"), itoa(mppClient.get.variableData.trackertemp, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_Inverter_Temperature"), itoa(mppClient.get.variableData.InverterTemp, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_Battery_Temperature"), itoa(mppClient.get.variableData.batteryTemp, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_Transformer_Temperature"), itoa(mppClient.get.variableData.transformerTemp, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_FAN_Speed"), itoa(mppClient.get.variableData.fanSpeed, msgBuffer, 10));
      // mqttclient.publish(topicBuilder(buff, "Q1/SCC Charge Power"), itoa(mppClient.get.variableData.sccChargePower, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Battery_Charger_Status"), mppClient.get.variableData.inverterChargeStatus);
    }
    // QPIGS
    if (mppClient.qAvaible.qpigs)
    {
      mqttclient.publish(topicBuilder(buff, "AC_in_Voltage"), dtostrf(mppClient.get.variableData.gridVoltage, 5, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "AC_in_Frequenz"), dtostrf(mppClient.get.variableData.gridFrequency, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "AC_out_Voltage"), dtostrf(mppClient.get.variableData.acOutputVoltage, 5, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "AC_out_Frequenz"), dtostrf(mppClient.get.variableData.acOutputFrequency, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "AC_out_VA"), itoa(mppClient.get.variableData.acOutputApparentPower, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "AC_out_Watt"), itoa(mppClient.get.variableData.acOutputActivePower, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "AC_out_percent"), itoa(mppClient.get.variableData.outputLoadPercent, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_Bus_Voltage"), itoa(mppClient.get.variableData.busVoltage, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Inverter_Bus_Temperature"), itoa(mppClient.get.variableData.inverterHeatSinkTemperature, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Battery_Voltage"), dtostrf(mppClient.get.variableData.batteryVoltage, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Battery_Percent"), itoa(mppClient.get.variableData.batteryCapacity, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Battery_Load"), itoa(mppClient.get.variableData.batteryLoad, msgBuffer, 10));
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

      if (mppClient.get.variableData.pvChargingPower[1] != -1)
      {
        for (size_t i : mppClient.get.variableData.pvChargingPower)
        {
          if (mppClient.get.variableData.pvChargingPower[i] != -1)
            mqttclient.publish(topicBuilder(buff, "PV_Watt" + i), String(mppClient.get.variableData.pvChargingPower[i]).c_str());
        }
      }
      else if (mppClient.get.variableData.pvChargingPower[0] != -1)
      {
        if (mppClient.get.variableData.pvChargingPower[0] != -1)
          mqttclient.publish(topicBuilder(buff, "PV_Watt"), String(mppClient.get.variableData.pvChargingPower[0]).c_str());
      }
    }
    // QMOD
    if (mppClient.qAvaible.qmod)
    {
      mqttclient.publish(topicBuilder(buff, "Inverter_Operation_Mode"), mppClient.get.variableData.operationMode);
    }
    // QALL
    if (mppClient.qAvaible.qall)
    {
      mqttclient.publish(topicBuilder(buff, "PV_generation_day"), itoa(mppClient.get.variableData.pvGenerationDay, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "PV_generation_sum"), itoa(mppClient.get.variableData.pvGenerationSum, msgBuffer, 10));
    }
    // QPIRI
    if (mppClient.qAvaible.qpiri)
    {
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_in_rating_voltage"), dtostrf(mppClient.get.staticData.gridRatingVoltage, 5, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_in_rating_current"), dtostrf(mppClient.get.staticData.gridRatingCurrent, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_out_rating_voltage"), dtostrf(mppClient.get.staticData.acOutputRatingVoltage, 5, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_out_rating_frequency"), dtostrf(mppClient.get.staticData.acOutputRatingFrquency, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_out_rating_current"), dtostrf(mppClient.get.staticData.acoutputRatingCurrent, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_out_rating_apparent_power"), itoa(mppClient.get.staticData.acOutputRatingApparentPower, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Device_Data/AC_out_rating_active_power"), itoa(mppClient.get.staticData.acOutputRatingActivePower, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_rating_voltage"), dtostrf(mppClient.get.staticData.batteryRatingVoltage, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_re-charge_voltage"), dtostrf(mppClient.get.staticData.batteryReChargeVoltage, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_under_voltage"), dtostrf(mppClient.get.staticData.batteryUnderVoltage, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_bulk_voltage"), dtostrf(mppClient.get.staticData.batteryBulkVoltage, 4, 1, msgBuffer));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_float_voltage"), dtostrf(mppClient.get.staticData.batteryFloatVoltage, 4, 1, msgBuffer));

      mqttclient.publish(topicBuilder(buff, "Device_Data/Battery_type"), mppClient.get.staticData.batterytype);
      mqttclient.publish(topicBuilder(buff, "Device_Data/Current_max_AC_charging_current"), itoa(mppClient.get.staticData.currentMaxAcChargingCurrent, msgBuffer, 10));
      mqttclient.publish(topicBuilder(buff, "Device_Data/Current_max_charging_current"), itoa(mppClient.get.staticData.currentMaxChargingCurrent, msgBuffer, 10));
    }
    // QPI
    if (mppClient.qAvaible.qpi)
    {
      mqttclient.publish(topicBuilder(buff, "Device_Data/Protocol_ID"), mppClient.get.staticData.deviceProtocol.c_str());
    }
    // QMN
    if (mppClient.qAvaible.qmn)
    {
      mqttclient.publish(topicBuilder(buff, "Device_Data/Device_Model"), mppClient.get.staticData.modelName.c_str());
    }
    */
// RAW
#ifdef DEBUG
    mqttclient.publish(topicBuilder(buff, "RAW/QPIGS"), (mppClient.get.raw.qpigs).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QPIRI"), (mppClient.get.raw.qpiri).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QPI"), (mppClient.get.raw.qpi).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QMOD"), (mppClient.get.raw.qmod).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QALL"), (mppClient.get.raw.qall).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QMN"), (mppClient.get.raw.qmn).c_str());
#endif
  }
  else
  {
    mqttclient.beginPublish(topicBuilder(buff, "Data"), measureJson(Json), false);
    serializeJson(Json, mqttclient);
    mqttclient.endPublish();
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
    // not needed anymore, we make a callback with the raw mqtt point
    // mqttclient.publish(topicBuilder(buff, "Device_Control/Set_Command_answer"), customResponse.c_str());
    valChange = true;
  }
}
