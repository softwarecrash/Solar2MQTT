#ifndef INVERTER_H
#define INVERTER_H

#include <Arduino.h>

void serviceInverter();
void requestInverterCommand(String command);

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

#endif
