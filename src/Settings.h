#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1024
// Settings: Stores persistant settings, loads and saves to EEPROM

#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1024

class Settings
{
  // change eeprom config version ONLY when new parameter is added and need reset the parameter
  unsigned int configVersion = 10;

public:
  String deviceNameStr;
  struct Data
  {                              // do not re-sort this struct
    unsigned int coVers;         // config version, if changed, previus config will erased
    char deviceName[40];         // device name
    char mqttServer[40];         // mqtt Server adress
    char mqttUser[40];           // mqtt Username
    char mqttPassword[40];       // mqtt Password
    char mqttTopic[40];          // mqtt publish topic
    char mqttTriggerPath[80];    // MQTT Data Trigger Path
    unsigned int mqttPort;       // mqtt port
    unsigned int mqttRefresh;    // mqtt refresh time
    unsigned int deviceQuantity; // Quantity of Devices
    bool mqttJson;               // switch between classic mqtt and json
    bool webUIdarkmode;          // Flag for color mode in webUI
  } data;

  void load()
  {
    data = {}; // clear bevor load data
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, data);
    EEPROM.end();
    coVersCheck();
    sanitycheck();
    deviceNameStr = data.deviceName;
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
      strcpy(data.deviceName, "Solar2MQTT");
    }
    if (strlen(data.mqttServer) == 0 || strlen(data.mqttServer) >= 40)
    {
      strcpy(data.mqttServer, "");
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
      strcpy(data.mqttTopic, "Solar");
    }
    if (data.mqttPort <= 0 || data.mqttPort >= 65530)
    {
      data.mqttPort = 0;
    }
    if (data.mqttRefresh <= 1 || data.mqttRefresh >= 65530)
    {
      data.mqttRefresh = 0;
    }
    if (data.mqttJson && !data.mqttJson)
    {
      data.mqttJson = false;
    }
    if (data.deviceQuantity < 1 || data.deviceQuantity >= 10)
    {
      data.deviceQuantity = 1;
    }
    if (strlen(data.mqttTriggerPath) == 0 || strlen(data.mqttTriggerPath) >= 80)
    {
      strcpy(data.mqttTriggerPath, "");
    }
    if (data.webUIdarkmode && !data.webUIdarkmode)
    {
      data.mqttJson = false;
    }
  }
  void coVersCheck()
  {
    if (data.coVers != configVersion)
    {
      data.coVers = configVersion;
      strcpy(data.deviceName, "Solar2MQTT");
      strcpy(data.mqttServer, "");
      strcpy(data.mqttUser, "");
      strcpy(data.mqttPassword, "");
      strcpy(data.mqttTopic, "Solar");
      strcpy(data.mqttTriggerPath, "");
      data.deviceQuantity = 1;
      data.mqttPort = 0;
      data.mqttRefresh = 300;
      data.mqttJson = false;
      data.webUIdarkmode = false;

      save();
      load();
    }
  }
};
