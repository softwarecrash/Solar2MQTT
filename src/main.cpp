
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
unsigned long mqtttimer = 0;
unsigned long RestartTimer = 0;
unsigned long slowDownTimer = 0;
bool shouldSaveConfig = false;
char mqtt_server[40];
bool restartNow = false;
bool askInverterOnce = false;
bool workerCanRun = true;
bool publishFirst = false;
bool haDiscTrigger = false;
unsigned int jsonSize = 0;
uint32_t bootcount = 0;
String commandFromUser;
String customResponse;

bool firstPublish;
DynamicJsonDocument Json(JSON_BUFFER); // main Json
// StaticJsonDocument <JSON_BUFFER>Json;
JsonObject deviceJson = Json.createNestedObject("EspData");    // basic device data
JsonObject staticData = Json.createNestedObject("DeviceData"); // battery package data
JsonObject liveData = Json.createNestedObject("LiveData");     // battery package data
// JsonObject rawData = Json.createNestedObject("RawData");       // battery package data

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
    size_t len = measureJson(liveData);
    AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
    if (buffer)
    {
      serializeJson(liveData, (char *)buffer->get(), len + 1);
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
    // mqtttimer = millis();
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    wsClient = nullptr;
    ws.cleanupClients(); // clean unused client connections
    break;
  }
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len)
{
  String d = "";
  for (uint i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  commandFromUser = (d);
  WebSerial.println("Sending [" + d + "] to Device");
}

bool resetCounter(bool count)
{

  if (count)
  {
    if (ESP.getResetInfoPtr()->reason == 6)
    {
      ESP.rtcUserMemoryRead(16, &bootcount, sizeof(bootcount));

      if (bootcount >= 10 && bootcount < 20)
      {
        // bootcount = 0;
        // ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
        settings.reset();
        ESP.eraseConfig();
        ESP.reset();
      }
      else
      {
        bootcount++;
        ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
      }
    }
    else
    {
      bootcount = 0;
      ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
    }
  }
  else
  {
    bootcount = 0;
    ESP.rtcUserMemoryWrite(16, &bootcount, sizeof(bootcount));
  }
  DEBUG_PRINT(F("Bootcount: "));
  DEBUG_PRINTLN(bootcount);
  DEBUG_PRINT(F("Reboot reason: "));
  DEBUG_PRINTLN(ESP.getResetInfoPtr()->reason);
  return true;
}

void setup()
{
  analogWrite(LED_PIN, 0);
  analogWrite(LED_COM, 0);
  analogWrite(LED_SRV, 0);
  analogWrite(LED_NET, 0);

  resetCounter(true);
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
  // wm.setConnectTimeout(15);       // how long to try to connect for before continuing
  // wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
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

  wm.setDebugOutput(false);       // disable wifimanager debug output
  wm.setMinimumSignalQuality(20); // filter weak wifi signals
  // wm.setConnectTimeout(15);       // how long to try to connect for before continuing
  // wm.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wm.setSaveConfigCallback(saveConfigCallback);
  // save settings if wifi setup is fire up
  bool apRunning = wm.autoConnect("Solar2MQTT-AP");
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
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
              if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
              AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_MAIN, htmlProcessor);
              request->send(response); });

    server.on("/livejson", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                serializeJson(Json, *response);
                request->send(response); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_REBOOT, htmlProcessor);
                request->send(response);
                restartNow = true;
                RestartTimer = millis(); });

    server.on("/confirmreset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_CONFIRM_RESET, htmlProcessor);
                request->send(response); });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
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
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS_EDIT, htmlProcessor);
                request->send(response); });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_SETTINGS, htmlProcessor);
                request->send(response); });

    server.on("/settingssave", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
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
                strncpy(settings.data.httpUser, request->arg("post_httpUser").c_str(), 40);
                strncpy(settings.data.httpPass, request->arg("post_httpPass").c_str(), 40);
                settings.data.haDiscovery = (request->arg("post_hadiscovery") == "true") ? true : false;
                settings.save();
                request->redirect("/reboot"); });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
                AsyncWebParameter *p = request->getParam(0);
                if (p->name() == "CC")
                {
                  commandFromUser = (p->value());
                }
                if (p->name() == "ha")
                {
                haDiscTrigger = true;
                }
                request->send(200, "text/plain", "message received"); });

    server.on(
        "/update", HTTP_POST, [](AsyncWebServerRequest *request)
        {
          if(strlen(settings.data.httpUser) > 0 && !request->authenticate(settings.data.httpUser, settings.data.httpPass)) return request->requestAuthentication();
       //To upload through terminal you can use: curl -F "image=@firmware.bin" <ESP_IP>/update

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
    WebSerial.begin(&server);
    WebSerial.onMessage(recvMsg);

    server.begin();

    mppClient.callback(prozessData);
    mppClient.Init(); // init the PI_serial Library

    mqtttimer = (settings.data.mqttRefresh * 1000) * (-1);
  }
  analogWrite(LED_PIN, 255);
  analogWrite(LED_COM, 255);
  analogWrite(LED_SRV, 255);
  analogWrite(LED_NET, 255);
  resetCounter(false);
}

void loop()
{
  if (Update.isRunning())
  {
    workerCanRun = false; // lockout, atfer true need reboot
  }
  if (workerCanRun)
  {
    // Make sure wifi is in the right mode
    if (WiFi.status() == WL_CONNECTED)
    { // No use going to next step unless WIFI is up and running.
      if (commandFromUser != "")
      {
        DEBUG_PRINTLN(commandFromUser);
        DEBUG_WEBLN(commandFromUser);
        String tmp = mppClient.sendCommand(commandFromUser); // send a custom command to the device
        DEBUG_PRINTLN(tmp);
        DEBUG_WEBLN(tmp);
        commandFromUser = "";
        mqtttimer = 0;
      }
      ws.cleanupClients(); // clean unused client connections
      MDNS.update();
      // getJsonData();
      mppClient.loop(); // Call the PI Serial Library loop
      mqttclient.loop();
      if ((haDiscTrigger || settings.data.haDiscovery) && measureJson(Json) > jsonSize)
      {
        if(sendHaDiscovery()){
        haDiscTrigger = false;
        //haAutoDiscTrigger = false;
        jsonSize = measureJson(Json);
        }
      }
    }
  }
  if (restartNow && millis() >= (RestartTimer + 500))
  {
    DEBUG_PRINTLN("Restart");
    DEBUG_WEBLN("Restart");
    ESP.restart();
  }
  notificationLED(); // notification LED routine
}

bool prozessData()
{
  if (millis() < (slowDownTimer + 1000) && mppClient.protocol == 0)
  {
    return true;
  }
  DEBUG_PRINTLN("ProzessData called");
  DEBUG_PRINTLN(mppClient.protocol);
  DEBUG_PRINTLN(mppClient.connection);
  getJsonData();
  if (wsClient != nullptr && wsClient->canSend())
  {
    notifyClients();
  }
  if (millis() - mqtttimer > (settings.data.mqttRefresh * 1000) || mqtttimer == 0)
  {
    sendtoMQTT(); // Update data to MQTT server if we should
    mqtttimer = millis();
  }

  slowDownTimer = millis();
  return true;
}

void getJsonData()
{
  deviceJson[F("Device_name")] = settings.data.deviceName;
  deviceJson[F("ESP_VCC")] = ESP.getVcc() / 1000.0;
  deviceJson[F("Wifi_RSSI")] = WiFi.RSSI();
  deviceJson[F("sw_version")] = SOFTWARE_VERSION;

  deviceJson[F("Free_Heap")] = ESP.getFreeHeap();
  deviceJson[F("HEAP_Fragmentation")] = ESP.getHeapFragmentation();
  deviceJson[F("json_memory_usage")] = Json.memoryUsage();
#ifdef isDEBUG
  deviceJson[F("json_capacity")] = Json.capacity();
  deviceJson[F("runtime")] = millis() / 1000;
  deviceJson[F("ws_clients")] = ws.count();
  deviceJson[F("ws_clients")] = mppClient.protocol;
#endif
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

    if (mqttclient.connect(mqttClientId, settings.data.mqttUser, settings.data.mqttPassword, (topicBuilder(buff, "Alive")), 0, true, "false", true))
    {
      if (mqttclient.connected())
      {

        mqttclient.publish(topicBuilder(buff, "Alive"), "true", true); // LWT online message must be retained!
        mqttclient.publish(topicBuilder(buff, "IP"), (const char *)(WiFi.localIP().toString()).c_str(), true);
        mqttclient.subscribe(topicBuilder(buff, "DeviceControl/Set_Command"));

        if (strlen(settings.data.mqttTriggerPath) >= 1)
        {
          mqttclient.subscribe(settings.data.mqttTriggerPath);
        }
        DEBUG_PRINTLN(F("Done"));
        DEBUG_WEBLN(F("Done"));
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
  if (!settings.data.mqttJson)
  {
    char msgBuffer1[200];
    for (JsonPair jsonDev : Json.as<JsonObject>())
    {
      for (JsonPair jsondat : jsonDev.value().as<JsonObject>())
      {
        sprintf(msgBuffer1, "%s/%s/%s", settings.data.mqttTopic, jsonDev.key().c_str(), jsondat.key().c_str());
        mqttclient.publish(msgBuffer1, jsondat.value().as<String>().c_str());
        //Serial.println(msgBuffer1);
      }
    }
    if (mppClient.get.raw.commandAnswer.length() > 0)
    {
      mqttclient.publish((String(settings.data.mqttTopic) + String("/DeviceControl/Set_Command_answer")).c_str(), (mppClient.get.raw.commandAnswer).c_str());
      DEBUG_PRINTLN(mppClient.get.raw.commandAnswer);
      DEBUG_WEBLN(mppClient.get.raw.commandAnswer);
      mppClient.get.raw.commandAnswer = "";
    }
// RAW
#ifdef DEBUG
    mqttclient.publish(topicBuilder(buff, "RAW/Q1"), (mppClient.get.raw.q1).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QPIGS"), (mppClient.get.raw.qpigs).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QPIGS2"), (mppClient.get.raw.qpigs2).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QPIRI"), (mppClient.get.raw.qpiri).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QT"), (mppClient.get.raw.qt).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QET"), (mppClient.get.raw.qet).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QEY"), (mppClient.get.raw.qey).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QEM"), (mppClient.get.raw.qem).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QED"), (mppClient.get.raw.qed).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QLT"), (mppClient.get.raw.qt).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QLY"), (mppClient.get.raw.qly).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QLM"), (mppClient.get.raw.qlm).c_str());
    mqttclient.publish(topicBuilder(buff, "RAW/QLD"), (mppClient.get.raw.qld).c_str());
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
  mqttclient.publish(topicBuilder(buff, "Alive"), "true", true); // LWT online message must be retained!
  mqttclient.publish(topicBuilder(buff, "EspData/Wifi_RSSI"), String(WiFi.RSSI()).c_str());
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
  if (strlen(settings.data.mqttTriggerPath) > 0 && strcmp(top, settings.data.mqttTriggerPath) == 0)
  {
    DEBUG_PRINTLN(F("<MQTT> MQTT Data Trigger Firered Up"));
    DEBUG_WEBLN(F("<MQTT> MQTT Data Trigger Firered Up"));
    // mqtttimer = 0;
    mqtttimer = (settings.data.mqttRefresh * 1000) * (-1);
  }

  if (messageTemp == "NAK" || messageTemp == "(NAK" || messageTemp == "")
    return;

  // send raw control command
  if (strcmp(top, topicBuilder(buff, "DeviceControl/Set_Command")) == 0 && messageTemp.length() > 0)
  {
    DEBUG_PRINT(F("Send Command message recived: "));
    DEBUG_PRINTLN(messageTemp);
    DEBUG_WEB(F("Send Command message recived: "));
    DEBUG_WEBLN(messageTemp);
    commandFromUser = messageTemp;
  }
}

bool sendHaDiscovery()
{
  if (!connectMQTT())
  {
    return false;
  }
  String haDeviceDescription = String("\"dev\":") +
                               "{\"ids\":[\"" + mqttClientId + "\"]," +
                               "\"name\":\"" + settings.data.deviceName + "\"," +
                               "\"cu\":\"http://" + WiFi.localIP().toString() + "\"," +
                               "\"mdl\":\"" + staticData["Device_Model"].as<String>().c_str() + "\"," +
                               "\"mf\":\"SoftWareCrash\"," +
                               "\"sw\":\"" + SOFTWARE_VERSION + "\"" +
                               "}";

  char topBuff[128];
  // char configBuff[1024];
  // size_t mqttContentLength;
  for (size_t i = 0; i < sizeof haStaticDescriptor / sizeof haStaticDescriptor[0]; i++)
  {
    if (staticData.containsKey(haStaticDescriptor[i][0]))
    {
      String haPayLoad = String("{") +
                         "\"name\":\"" + haStaticDescriptor[i][0] + "\"," +
                         "\"stat_t\":\"" + settings.data.mqttTopic + "/DeviceData/" + haStaticDescriptor[i][0] + "\"," +
                         "\"uniq_id\":\"" + mqttClientId + "." + haStaticDescriptor[i][0] + "\"," +
                         "\"ic\":\"mdi:" + haStaticDescriptor[i][1] + "\",";
      if (strlen(haStaticDescriptor[i][2]) != 0)
        haPayLoad += (String) "\"unit_of_meas\":\"" + haStaticDescriptor[i][2] + "\",";
      if (strlen(haStaticDescriptor[i][3]) != 0)
        haPayLoad += (String) "\"dev_cla\":\"" + haStaticDescriptor[i][3] + "\",";
      haPayLoad += haDeviceDescription;
      haPayLoad += "}";
      sprintf(topBuff, "homeassistant/sensor/%s/%s/config", settings.data.deviceName, haStaticDescriptor[i][0]); // build the topic
      mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
      for (size_t i = 0; i < haPayLoad.length(); i++)
      {
        mqttclient.write(haPayLoad[i]);
      }
      mqttclient.endPublish();

      /*
            sprintf(topBuff, "homeassistant/sensor/%s/%s/config", settings.data.deviceName, haStaticDescriptor[i][0]); // build the topic
            mqttContentLength = sprintf(configBuff, "{\"state_topic\": \"%s/DeviceData/%s\",\"unique_id\": \"sensor.%s_%s\",\"name\": \"%s\",\"icon\": \"mdi:%s\",\"unit_of_measurement\": \"%s\",\"device_class\":\"%s\",\"device\":{\"identifiers\":[\"%s\"], \"configuration_url\":\"http://%s\",\"name\":\"%s\", \"model\":\"%s\",\"manufacturer\":\"SoftWareCrash\",\"sw_version\":\"Solar2MQTT %s\"}}",
                                        settings.data.mqttTopic, haStaticDescriptor[i][0], settings.data.deviceName, haStaticDescriptor[i][0], haStaticDescriptor[i][0], haStaticDescriptor[i][1], haStaticDescriptor[i][2], haStaticDescriptor[i][3], staticData["Serial_number"].as<String>().c_str(), (const char *)(WiFi.localIP().toString()).c_str(), settings.data.deviceName, staticData["Device_Model"].as<String>().c_str(), SOFTWARE_VERSION);
            mqttclient.beginPublish(topBuff, mqttContentLength, true);
            for (size_t i = 0; i < mqttContentLength; i++)
            {
              mqttclient.write(configBuff[i]);
            }
            mqttclient.endPublish();
            */
    }
  }

  for (size_t i = 0; i < sizeof haLiveDescriptor / sizeof haLiveDescriptor[0]; i++)
  {
    if (liveData.containsKey(haLiveDescriptor[i][0]))
    {
      String haPayLoad = String("{") +
                         "\"name\":\"" + haLiveDescriptor[i][0] + "\"," +
                         "\"stat_t\":\"" + settings.data.mqttTopic + "/LiveData/" + haLiveDescriptor[i][0] + "\"," +
                         "\"uniq_id\":\"" + mqttClientId + "." + haLiveDescriptor[i][0] + "\"," +
                         "\"ic\":\"mdi:" + haLiveDescriptor[i][1] + "\",";
      if (strlen(haLiveDescriptor[i][2]) != 0)
        haPayLoad += (String) "\"unit_of_meas\":\"" + haLiveDescriptor[i][2] + "\",";
      if (strlen(haLiveDescriptor[i][3]) != 0)
        haPayLoad += (String) "\"dev_cla\":\"" + haLiveDescriptor[i][3] + "\",";
      haPayLoad += haDeviceDescription;
      haPayLoad += "}";
      sprintf(topBuff, "homeassistant/sensor/%s/%s/config", settings.data.deviceName, haLiveDescriptor[i][0]); // build the topic
      mqttclient.beginPublish(topBuff, haPayLoad.length(), true);
      for (size_t i = 0; i < haPayLoad.length(); i++)
      {
        mqttclient.write(haPayLoad[i]);
      }
      mqttclient.endPublish();

      /*
            sprintf(topBuff, "homeassistant/sensor/%s/%s/config", settings.data.deviceName, haLiveDescriptor[i][0]); // build the topic
            mqttContentLength = sprintf(configBuff, "{\"state_topic\": \"%s/LiveData/%s\",\"unique_id\": \"sensor.%s_%s\",\"name\": \"%s\",\"icon\": \"mdi:%s\",\"unit_of_measurement\": \"%s\",\"device_class\":\"%s\",\"device\":{\"identifiers\":[\"%s\"], \"configuration_url\":\"http://%s\",\"name\":\"%s\", \"model\":\"%s\",\"manufacturer\":\"SoftWareCrash\",\"sw_version\":\"Solar2MQTT %s\"}}",
                                        settings.data.mqttTopic, haLiveDescriptor[i][0], settings.data.deviceName, haLiveDescriptor[i][0], haLiveDescriptor[i][0], haLiveDescriptor[i][1], haLiveDescriptor[i][2], haLiveDescriptor[i][3], staticData["Serial_number"].as<String>().c_str(), (const char *)(WiFi.localIP().toString()).c_str(), settings.data.deviceName, staticData["Device_Model"].as<String>().c_str(), SOFTWARE_VERSION);
            mqttclient.beginPublish(topBuff, mqttContentLength, true);
            for (size_t i = 0; i < mqttContentLength; i++)
            {
              mqttclient.write(configBuff[i]);
            }
            mqttclient.endPublish();
            */
    }
  }
  return true;
}