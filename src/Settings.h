/*
DALY BMS to MQTT Project
https://github.com/softwarecrash/DALY-BMS-to-MQTT
This code is free for use without any waranty.
when copy code or reuse make a note where the codes comes from.
*/

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1024

class Settings
{
  // change eeprom config version ONLY when new parameter is added and need reset the parameter
  unsigned int configVersion = 10;

public:
  struct Data
  {                             // do not re-sort this struct
    unsigned int coVers;        // config version, if changed, previus config will erased
    char deviceName[40];        // device name
    char mqttServer[40];        // mqtt Server adress
    char mqttUser[40];          // mqtt Username
    char mqttPassword[40];      // mqtt Password
    char mqttTopic[40];         // mqtt publish topic
    unsigned int mqttPort;      // mqtt port
    unsigned int mqttRefresh;   // mqtt refresh time
    bool mqttJson;              // switch between classic mqtt and json
    bool wakeupEnable;  // use wakeup output?
    bool relaisFailsafe; // relais failsafe mode | false - turn off, true - keep last state
    bool relaisEnable;  // enable relais output?
    bool relaisInvert;  // invert relais output?
    byte relaisFunction;    // function mode - 0 = Lowest Cell Voltage, 1 = Highest Cell Voltage, 2 = Pack Cell Voltage, 3 = Temperature
    byte relaisComparsion;  // comparsion mode - 0 = Higher or equal than, 1 = Lower or equal than
    float relaisSetValue; // value to compare to !!RENAME TO SOMETHING BETTER!!
    float relaisHysteresis; // value to compare to
  } data;

  void load()
  {
    data = {}; // clear bevor load data
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, data);
    EEPROM.end();
    coVersCheck();
    sanitycheck();
  }

  void save()
  {
    sanitycheck();
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, data);
    EEPROM.commit();
    EEPROM.end();
  }

  void reset()
  {
    data = {};
    save();
  }

private:
  // check the variables from eeprom

  void sanitycheck()
  {
    if (strlen(data.deviceName) == 0 || strlen(data.deviceName) >= 40)
    {
      strcpy(data.deviceName, "DALY-BMS-to-MQTT");
    }
    if (strlen(data.mqttServer) == 0 || strlen(data.mqttServer) >= 40)
    {
      strcpy(data.mqttServer, "-1");
    }
    if (strlen(data.mqttUser) == 0 || strlen(data.mqttUser) >= 40)
    {
      strcpy(data.mqttUser, "");
    }
    if (strlen(data.mqttPassword) == 0 || strlen(data.mqttPassword) >= 40)
    {
      strcpy(data.mqttPassword, "");
    }
    if (strlen(data.mqttTopic) == 0 || strlen(data.mqttTopic) >= 40)
    {
      strcpy(data.mqttTopic, "BMS01");
    }
    if (data.mqttPort <= 0 || data.mqttPort >= 65530)
    {
      data.mqttPort = 0;
    }
    if (data.mqttRefresh <= 1 || data.mqttRefresh >= 65530)
    {
      data.mqttRefresh = 1;
    }
    if (data.mqttJson && !data.mqttJson)
    {
      data.mqttJson = false;
    }
    if (data.wakeupEnable && !data.wakeupEnable)
    {
      data.wakeupEnable = false;
    }
    if (data.relaisFailsafe && !data.relaisFailsafe)
    {
      data.relaisFailsafe = false;
    }
    if (data.relaisEnable && !data.relaisEnable)
    {
      data.relaisEnable = false;
    }
    if (data.relaisInvert && !data.relaisInvert)
    {
      data.relaisInvert = false;
    }
    if (data.relaisFunction < 0 || data.relaisFunction > 4)
    {
      data.relaisFunction = 0;
    }
    if (data.relaisComparsion < 0 || data.relaisComparsion > 1)
    {
      data.relaisComparsion = 0;
    }
    if (data.relaisSetValue < -100 || data.relaisSetValue > 100)
    {
      data.relaisSetValue = 0;
    }
    if (data.relaisHysteresis < -100 || data.relaisHysteresis > 100)
    {
      data.relaisHysteresis = 0;
    }
  }
  void coVersCheck()
  {
    if (data.coVers != configVersion)
    {
      data.coVers = configVersion;
      strcpy(data.deviceName, "DALY-BMS-to-MQTT");
      strcpy(data.mqttServer, "-1");
      strcpy(data.mqttUser, "");
      strcpy(data.mqttPassword, "");
      strcpy(data.mqttTopic, "BMS01");
      data.mqttPort = 0;
      data.mqttRefresh = 300;
      data.mqttJson = false;
      data.wakeupEnable = false;
      data.relaisFailsafe = false;
      data.relaisEnable = false;
      data.relaisInvert = false;
      data.relaisFunction = 0;
      data.relaisComparsion = 0;
      data.relaisSetValue = 0.0;
      data.relaisHysteresis = 0.0;
      save();
      load();
    }
  }
};








/*
//Settings: Stores persistant settings, loads and saves to EEPROM

#include <Arduino.h>
#include <EEPROM.h>

//#define SERIALDEBUG //enable Serial Output for debug
#define MQTTDEBUG //enable debug / RAW Messages from iverter to MQTT

class Settings
{
public:
  bool _valid = false;
  //MQTT Settings
  String _deviceType = "PIP"; //PIP | PCM | MPI
  String _deviceName = "";    //name of the device
  String _mqttServer = "";    //host or ip from the mqtt server
  String _mqttUser = "";      //mqtt username to login
  String _mqttPassword = "";  //mqtt passwort
  String _mqttTopic = "";     //MQTT Topic
  short _mqttPort = 0;        //mqtt server port
  short _mqttRefresh = 0;     //mqtt Send Interval in Seconds

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

  void readString(String &s, int maxLen, int offset)
  {
    int i;
    s = "";
    for (i = 0; i < maxLen; ++i)
    {
      char c = EEPROM.read(offset + i);
      if (c == 0)
        break;
      s += c;
    }
  }

  void writeString(String &s, int maxLen, int offset)
  {
    int i;
    //leave space for null termination
    maxLen--;
    if ((int)s.length() < maxLen - 1)
      maxLen = s.length();
    #ifdef SERIALDEBUG
    Serial1.print("Writing ");
    Serial1.print(maxLen);
    Serial1.print(" ");
    Serial1.print(offset);
    Serial1.print(" ");
    Serial1.println(s);
    #endif

    for (i = 0; i < maxLen; ++i)
    {
      EEPROM.write(offset + i, s[i]);
      #ifdef SERIALDEBUG
      Serial1.print(" ");
      Serial1.print(offset + i);
      Serial1.print("=");
      Serial1.print(s[i]);
      Serial1.print(",");
      #endif
    }
    //null terminate the string
    EEPROM.write(offset + i, 0);
    #ifdef SERIALDEBUG
    Serial1.print(offset + i);
    #endif
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
      _mqttRefresh = readShort(0x20);
      readString(_mqttTopic, 0x20, 0x40);
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

    writeShort(_mqttRefresh, 0x20);
    writeString(_mqttTopic, 0x20, 0x40);
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

  void reset(){
  _deviceName = "";
  _mqttServer = "";
  _mqttUser = "";
  _mqttPassword = "";
  _mqttTopic = "";
  _mqttPort = 0;
  _mqttRefresh = 0;
  save();
  delay(500);
  }

  Settings()
  {
    load();
  }
};
*/