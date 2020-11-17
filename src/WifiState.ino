
//----------------------------------------------------------------------
// Configure wifi as access point to allow client config
void setupWifiAp()
{
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP(ApSsid);
}

//----------------------------------------------------------------------
void setupWifiStation()
{
  WiFi.disconnect();
  delay(20);
  if (_settings._wifiSsid.length() == 0)
  {
    Serial.println("No client SSID set, switching to AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ApSsid);
  }
  else
  {
    Serial.print("Connecting to ");
    Serial.print(_settings._wifiSsid);
    Serial.print(":");
    Serial.println(_settings._wifiPass);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_settings._wifiSsid.c_str(), _settings._wifiPass.c_str());
  }
}

//----------------------------------------------------------------------
void serviceWifiMode()
{
  if (clientReconnect)
  {
    WiFi.disconnect();
    delay(10);
    clientReconnect = false;
    currentApMode = CLIENT_NOTCONNECTED;
  }
  
  if (currentApMode != requestApMode)
  {
    if (requestApMode == WIFI_AP)
    {
      Serial.println("Access Point Mode");
      setupWifiAp();             
      currentApMode = WIFI_AP;
    }

    if (requestApMode == WIFI_STA)
    {
      Serial.println("Station Mode");
      setupWifiStation();             
      currentApMode = WIFI_STA;
      clientConnectionState = CLIENT_CONNECTING;
    }
  }  

  if (clientConnectionState == CLIENT_CONNECTING)
  {    
    Serial.print("connect ");
    Serial.println(WIFI_COUNT);
    WIFI_COUNT++;
    if (WIFI_COUNT > 200) { 
      ESP.restart();
      WIFI_COUNT=0;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("connect 2");
      clientConnectionState = CLIENT_CONNECTED;
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      WIFI_COUNT = 0;
    }
  }
}
