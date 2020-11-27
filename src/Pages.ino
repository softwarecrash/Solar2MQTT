//Pages: Holds pages to serve via HTTP

String getNextToken(String& s, int& offset)
{
  char c;
  String result = "";
  
  do
  {
    c = s[offset];
    ++offset;  

    if ((c != 0) && (c != '&') && (c != '?') && (c != ' ') && (c != '\r') && (c != '\n'))
    {
      result += c;
    }
    else
    {
      return result;
    }

  } while(offset < s.length());

  return result;
}

void appendHttp200(String& s)
{
  s += F("HTTP/1.1 200 OK\r\n");
  s += F("Content-Type: text/html\r\n\r\n");
  s += F("<!DOCTYPE HTML>\r\n<html>\r\n");
}

void serve404(WiFiClient& client)
{
  String s = F("HTTP/1.1 404 Not Found\r\n");
  s += F("Content-Type: text/html\r\n\r\n");
  s += F("<!DOCTYPE HTML>\r\n<html>\r\n");
  s += F("Page not found");
  s += F("</html>");

  client.print(s);
  delay(1);
  
}

//Wifi setup, requests AP list via AJAX
void serveWifiSetupPage(WiFiClient& client)
{
  String s = "";
  appendHttp200(s);

  //This thing was automatically generated from html source
  s += F("<div><H1>Wifi Setup</H1></div>\r\n<div id=\"id01\">Searching for networks...</div>\r\n</html>\r\n\r\n");
  s += F("<script>\r\n\tvar xmlhttp = new XMLHttpRequest();\r\n\tvar url = \"aplist.json\";\r\n\r\n\txmlhttp.onreadystatechange = function() {\r\n");
  s += F("\t\tif (xmlhttp.readyState == 4 && xmlhttp.status == 200) {\r\n\t\t\tvar myArr = JSON.parse(xmlhttp.responseText);\r\n\t\t\tmyFunction(myArr);");
  s += F("\r\n\t\t}\r\n\t}\r\n\txmlhttp.open(\"GET\", url, true);\r\n\txmlhttp.send();\r\n\r\n\tfunction myFunction(arr) {\r\n");
  s += F("\t\tvar out = \"<fieldset><legend>WiFi Network to connect to</legend>\";\r\n\t\tout += '<form action=setap method=GET><table style=\"width:80%\">';\r\n\t\tvar i;\r\n");
  s += F("\t\tfor(i = 0; i < arr.length; i++) \r\n\t\t{\r\n\t\t\tout += '<tr><td align=right><input type=radio name=ssid id=\"s' + i + '\" value=\"' + arr[i].n + '\"/></td>' \r\n");
  s += F("\t\t\t\t+ '<td><label for=s' + i + '>' + arr[i].n + ' (' + arr[i].r + 'db)</label></td>'\r\n\t\t\t\t+ '</tr>';\r\n\t\t}\r\n");
  //s += F("\t\tout += \"<tr><td align=right><input type=radio name=ssid id=sc value=''></td><td><input type=text value='Custom SSID' onfocus='document.getElementById(sc).checked=true'></input></input></td></tr>\";\r\n");
  s += F("\t\tout += \"<tr><td><br></td></tr>\"\r\n\t\tout += \"<tr><td width=1 align=right>Password:</td><td><input type=text name=pass></input></td></tr>\"\r\n\t\tout += \"<tr><td><br></td></tr>\"\r\n");
  s += F("\t\tout += \"<tr><td></td><td>Note: Password is sent over plaintext, only use on secure network.</td></tr>\"\r\n\t\tout += \"<tr><td></td><td><input type=submit value='Submit'/></td></tr>\"\r\n\t\t");
  s += F("\t\tout += '</table></form>'\r\n\t\tdocument.getElementById(\"id01\").innerHTML = out;\r\n\t}\r\n</script>");

  client.print(s);
  delay(1);
  
}

//AJAX reply with list of APs
void serveWifiApList(WiFiClient& client)
{
  String s = "";

  s += F("HTTP/1.1 200 OK\r\n");
  s += F("Content-Type: application/json; charset=utf-8\r\n");
  s += F("Access-Control-Allow-Origin: *\r\n\r\n");
  
  s += "[";
  int8_t n = WiFi.scanNetworks();
  for (int8_t i=0; i<n; i++)
  {
    s += "{\"n\":\"";
    s += WiFi.SSID(i);
    s += "\", \"r\":";
    s += String(WiFi.RSSI(i));
    s += "}";      
    if (i != n-1)
      s += ",";
  }
  s += "]";
  client.print(s);
  delay(1);

}

//Apply AP settings
void serveWifiSetAp(WiFiClient& client, String req)
{
  String s = "";
  appendHttp200(s);

  Serial1.println("Setting AP");
  Serial1.println(req);

  //"/setap?ssid=myssid&pass=mypass
  int index0 = req.indexOf("?ssid=", 3);
  if (index0 > 0)
  {
    Serial1.println(index0);
    int index1 = req.indexOf("&pass=", index0);
    if (index1 > 0)
    {
      Serial1.println(index1);
      int index2 = req.indexOf(" HTTP/", index1);
      if (index2 == -1)
        index2 = req.length();
      String ssid = req.substring(index0+6, index1);
      String pass = req.substring(index1+6, index2);

      Serial1.println(index2);
      Serial1.println(ssid);
      Serial1.println(pass);
      
      _settings._wifiSsid = ssid;
      _settings._wifiPass = pass;
      _settings.save();
    
      s += F("Set access point to ");
      s += ssid;
      s += ":";
      s += pass;
      s += F("<br>Switching to client mode, this connection has now closed. Hopefully you'll find me again on ");
      s += F("<br><br><a href=\"/\">Back</a><br>");
      s += ssid;     

      Serial1.println(s);
      requestApMode = WIFI_STA;
      clientReconnect = true;
      delay(1);
      //ESP.restart();
    }
  }

  
  //This thing was automatically generated from html source
  s += F("");
  s += F("</html>");

  client.print(s);
  delay(1);
  
}



//MQTT and device name
void serveMqtt(WiFiClient& client, String req)
{
  String s = "";
  appendHttp200(s);

  //This thing was automatically generated from html source
  s += F("<H1>MQTT server & device name</H1></html>\r\n\r\n");

  s += F("<form action=setmqtt>");
  s += F("<table>");
  s += F("<tr><td><b>Setting</b></td><td><b>Value Key</b></td></tr>");
  s += F("<tr><td>Mqtt Server</td><td><input type=text name=w width=20 value='");
  s += _settings._mqttServer;
  s += F("'></td><td><input type=text name=r width=20 value='");
  s += _settings._mqttPort;
  s += F("'></td></tr>");
  s += F("<tr><td>Mqtt User/pass</td><td><input type=text name=u width=20 value='");
  s += _settings._mqttUser;
  s += F("'></td><td><input type=text name=p width=20 value='");
  s += _settings._mqttPassword;
  s += F("'></td></tr>");
  
  s += F("<tr><td>Device type (MPI, PCM, PIP)</td><td><input type=text name=t width=20 value='");
  s += _settings._deviceType;
  s += F("'></td></tr>");
  
  s += F("<tr><td>Device Name</td><td><input type=text name=n width=20 value='");
  s += _settings._deviceName;
  s += F("'></td></tr>");
  
  s += F("<tr><td><br></td></tr>");
  
  s += F("<tr><td>Update rate</td><td><input type=text name=i width=5 value='");
  s += String(_settings._mqttPort);
  s += F("'></td><td>seconds</td></tr>");

  s += F("<tr><td></td><td></td><td><input type=submit value='    Save Settings   '></td></tr>");
  s += F("</table></form>");
  s += F("</html>\r\n\r\n");
  client.print(s);
  delay(1);
  
}

bool setStringIfStartsWith(String& s, String startswith, String& set)
{
  /*Serial1.print("  checking if ");
  Serial1.print(s);
  Serial1.print(" startswith ");
  Serial1.println(startswith);*/

  if (s.startsWith(startswith))
  {
    set = s.substring(startswith.length());
    Serial1.print("match >");
    Serial1.print(startswith);
    Serial1.print("< = >");
    Serial1.print(set);
    Serial1.println("<");

    return true;
  }
  return false;
}

//Apply mqtt settings
void serveSetMqtt(WiFiClient& client, String req)
{
  String s = "";

  Serial1.println("Setting MQTT & Device keys");
  Serial1.println(req);

  int offset = 0;
  String token = getNextToken(req, offset);

  while (token.length())
  {    
    setStringIfStartsWith(token, "w=", _settings._mqttServer);
    //setStringIfStartsWith(token, "r=", _settings._mqttPort);
    setStringIfStartsWith(token, "t=", _settings._deviceType);
    setStringIfStartsWith(token, "n=", _settings._deviceName);
    setStringIfStartsWith(token, "u=", _settings._mqttUser);
    setStringIfStartsWith(token, "p=", _settings._mqttPassword);
    if (setStringIfStartsWith(token, "r=", s))
      _settings._mqttPort = (short)s.toInt();
         
    token = getNextToken(req, offset);
  }

  _settings.save();
  
  s = "";
  appendHttp200(s);
  s += F("Saved");
  s += F("<br><br><a href=\"/\">Back</a><br>");
  
  s += F("");

  client.print(s);
  delay(1);
 
}

void servePage(WiFiClient& client, String req)
{
  String s = "";
  appendHttp200(s);

    s += F("<H1>Solar2MQTT</H1></html>\r\n\r\n");
    s += F("<a href=\"mqtt\">Configure MQTT</a><br>");
    s += F("<a href=\"wifi\">Configure Wifi</a><br><br><br>");
    s += F("<a href=\"reboot\">Reboot device</a><br>");
    s += F("<br><br><br><br><br>By Daromer aka Diy Tech & Repairs 2020<br>");
  
  /*int offset = 0;
  String token = getNextToken(req, offset);
  while (token.length())
  {    
    setStringIfStartsWith(token, "w=", _settings._mqttServer);
    setStringIfStartsWith(token, "r=", _settings._mqttPort);
    setStringIfStartsWith(token, "t=", _settings._deviceType);
    setStringIfStartsWith(token, "n=", _settings._deviceName);
    setStringIfStartsWith(token, "u=", _settings._mqttUser);
    setStringIfStartsWith(token, "p=", _settings._mqttPassword);
    if (setStringIfStartsWith(token, "i=", s))
      _settings._updateRateSec = (short)s.toInt();
         
    token = getNextToken(req, offset);
  }*/

  s += F("</html>\r\n\r\n");
  s += F("");

  client.print(s);
  delay(1);
}
