
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

void serveWifiSetAp(WiFiClient& client, String req)
{
  String s = "";
  appendHttp200(s);

  Serial.println("Setting AP");
  Serial.println(req);

  //"/setap?ssid=myssid&pass=mypass
  int index0 = req.indexOf("?ssid=", 3);
  if (index0 > 0)
  {
    Serial.println(index0);
    int index1 = req.indexOf("&pass=", index0);
    if (index1 > 0)
    {
      Serial.println(index1);
      int index2 = req.indexOf(" HTTP/", index1);
      if (index2 == -1)
        index2 = req.length();
      String ssid = req.substring(index0+6, index1);
      String pass = req.substring(index1+6, index2);

      Serial.println(index2);
      Serial.println(ssid);
      Serial.println(pass);
      
      _settings._wifiSsid = ssid;
      _settings._wifiPass = pass;
      _settings.save();
    
      s += F("Set access point to ");
      s += ssid;
      s += ":";
      s += pass;
      s += F("<br>Switching to client mode, this connection has now closed. Hopefully you'll find me again on ");
      s += ssid;     

      Serial.println(s);
      requestApMode = WIFI_STA;
      clientReconnect = true;
    }
  }

  
  //This thing was automatically generated from html source
  s += F("");
  s += F("</html>");

  client.print(s);
  delay(1);
  
}

