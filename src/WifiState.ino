
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
    ledMode = LED_MODE_SOLID_BOTH;
  }
  
  if (currentApMode != requestApMode)
  {
    if (requestApMode == WIFI_AP)
    {
      Serial.println("Access Point Mode");
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GRN_LED_PIN, LOW);
      setupWifiAp();             
      currentApMode = WIFI_AP;
      ledMode = LED_MODE_SOLID_RED; 
    }

    if (requestApMode == WIFI_STA)
    {
      Serial.println("Station Mode");
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GRN_LED_PIN, HIGH);
      setupWifiStation();             
      currentApMode = WIFI_STA;
      clientConnectionState = CLIENT_CONNECTING;
      ledMode = LED_MODE_FAST_GRN;    
    }
  }  

  if (clientConnectionState == CLIENT_CONNECTING)
  {    
    if (WiFi.status() == WL_CONNECTED)
    {
      clientConnectionState = CLIENT_CONNECTED;
      ledMode = LED_MODE_SOLID_GRN;
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }

}
