#ifndef INVERTER_H
#define INVERTER_H

#include <Arduino.h>

#define INVERTER_COMMAND_TIMEOUT_MS 20000
#define INVERTER_COMMAND_DELAY_MS 500

//Send and receive periodic inverter commands
void serviceInverter();

struct QpiMessage
{
  byte protocolId;
};


struct QpigsMessage
{
  unsigned long rxTimeSec;
  float gridV;
  float gridHz;
  float acOutV;
  float acOutHz;
  short acOutVa;
  short acOutW;
  byte acOutPercent;
  short busV;
  float battV;
  float battChargeA;
  float battPercent;
  float heatSinkDegC;
  float solarA;
  float solarV;
  float sccBattV;
  float battDischargeA;
  bool addSbuPriorityVersion;
  bool isConfigChanged;
  bool isSccFirmwareUpdated;
  bool isLoadOn; 
  bool battVoltageToSteadyWhileCharging;
  byte chargingStatus;
  byte reservedY;
  byte reservedZ;
  long reservedAA;
  short reservedBB;
}; 

struct QmodMessage
{
  char mode;
};

struct QpiwsMessage
{
  bool reserved0;
  bool inverterFault;
  bool busOver;
  bool busUnder;
  bool busSoftFail;
  bool lineFail;
  bool opvShort;
  bool overTemperature;
  bool fanLocked;
  bool batteryVoltageHigh;
  bool batteryLowAlarm;
  bool reserved13;
  bool batteryUnderShutdown;
  bool reserved15;
  bool overload;
  bool eepromFault;
  bool inverterOverCurrent;
  bool inverterSoftFail;
  bool selfTestFail;
  bool opDcVoltageOver;
  bool batOpen;
  bool currentSensorFail;
  bool batteryShort;
  bool powerLimit;
  bool pvVoltageHigh;
  bool mpptOverloadFault;
  bool mpptOverloadWarning;
  bool batteryTooLowToCharge;
  bool reserved30;
  bool reserved31;
};

struct QflagMessage
{
  bool disableBuzzer;
  bool enableOverloadBypass;
  bool enablePowerSaving;
  bool enableLcdEscape;
  bool enableOverloadRestart;
  bool enableOvertempRestart;
  bool enableBacklight;
  bool enablePrimarySourceInterruptedAlarm;
  bool enableFaultCodeRecording;
};

struct QidMessage
{
  char id[16];
};

#endif
