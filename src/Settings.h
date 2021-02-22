//Settings: Stores persistant settings, loads and saves to EEPROM

#include <arduino.h>
#include <EEPROM.h>

class Settings
{
  public:

    bool _valid = false;
    //WIFI Settings
    String _wifiSsid = "PC Hilfe Weimar";
    String _wifiPass = "pchwpchw";
  
  //MQTT Settings
    String _deviceType = "1";
    String _deviceName = "I solar";
    String _mqttServer = "192.168.178.40";
    //short _mqttPort = "1883";
    String _mqttUser = "mqttuser";
    String _mqttPassword = "123mqt456MQT";
    short  _mqttPort = 1883;


    short readShort(int offset)
    {
      byte b1 = EEPROM.read(offset + 0);
      byte b2 = EEPROM.read(offset + 1);
      return ((short)b1 << 8) | b2;
    }

    void writeShort(short value, int offset)
    {
      byte b1 = (byte)((value >> 8) & 0xFF); 
      byte b2 = (byte)((value >> 0) & 0xFF);
      
      EEPROM.write(offset + 0, b1);
      EEPROM.write(offset + 1, b2);
    }
    
    void readString(String& s, int maxLen, int offset)
    {
      int i;
      s = "";
      for (i=0; i<maxLen; ++i)
      {
        char c = EEPROM.read(offset+i);
        if (c == 0)
          break;
        s += c;
      }
    }
  
    void writeString(String& s, int maxLen, int offset)
    {
      int i;
      //leave space for null termination
      maxLen--;
      if (s.length() < maxLen-1)
        maxLen = s.length();

Serial.print("Writing ");
Serial.print(maxLen);
Serial.print(" ");
Serial.print(offset);
Serial.print(" ");
Serial.println(s);

      for (i=0; i<maxLen; ++i)
      {
        EEPROM.write(offset+i, s[i]);
        Serial.print(" ");
        Serial.print(offset+i);
        Serial.print("=");
        Serial.print(s[i]);
        Serial.print(",");
      }
      //null terminate the string
      EEPROM.write(offset+i, 0);
      Serial.print(offset+i);
    }
    
    void load()
    {
      EEPROM.begin(512);
  
      _valid = true;
      _valid &= EEPROM.read(0) == 0xDB;
      _valid &= EEPROM.read(1) == 0xEE;
      _valid &= EEPROM.read(2) == 0xAE;
      _valid &= EEPROM.read(3) == 0xDF;

      if (_valid)
      {
        readString(_wifiSsid, 0x20, 0x20);
        readString(_wifiPass, 0x20, 0x40);
        readString(_deviceType, 0x20, 0x60);
        readString(_deviceName, 0x20, 0x80);
        readString(_mqttServer, 0x20, 0xA0);
        readString(_mqttPassword, 0x20, 0xC0);
        readString(_mqttUser, 0x20, 0xE0);
        _mqttPort = readShort(0x100);
      }
      
      EEPROM.end();
    }
  
    void save()
    {
      EEPROM.begin(512);
  
      EEPROM.write(0, 0xDB);
      EEPROM.write(1, 0xEE);
      EEPROM.write(2, 0xAE);
      EEPROM.write(3, 0xDF);
  
      writeString(_wifiSsid, 0x20, 0x20);
      writeString(_wifiPass, 0x20, 0x40);
      writeString(_deviceType, 0x20, 0x60);
      writeString(_deviceName, 0x20, 0x80);
      writeString(_mqttServer, 0x20, 0xA0);
      writeString(_mqttPassword, 0x20, 0xC0);
      writeString(_mqttUser, 0x20, 0xE0);
      writeShort(_mqttPort, 0x100);

      EEPROM.commit();

      _valid = true;

      EEPROM.end();    
    }

    Settings()
    {
      load(); 
    }
};
