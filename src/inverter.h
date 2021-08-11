#ifndef INVERTER_H
#define INVERTER_H

#include <Arduino.h>

#define INVERTER_COMMAND_TIMEOUT_MS 5000
#define INVERTER_COMMAND_DELAY_MS 500

//Send and receive periodic inverter commands
void serviceInverter();

struct QpiMessage
{
  byte protocolId;
  //the inverter protocol id, is this the ident what the inverter answer?
};

struct P003PSMessage
{
  unsigned long rxTimeSec;
  float solarWatt1;
  float solarWatt2;
  float batteryWatt;
  float acin2_r;
  float acin2_s;
  float acin2_t;
  float acin2_total;
  float w_r;
  float w_s;
  float w_t;
  float w_total;
  float va_r;
  float va_s;
  float va_t;
  float va_total;
  float ac_output_procent;  
};

struct P006FPADJMessage
{  //-- '34'^D0301,0000,1,0099,1,0109,1,0112⸮7'
  unsigned long rxTimeSec;
  float dir;
  float watt;
  float feedingGridDirectionR;
  float calibrationWattR;  
  float feedingGridDirectionS;
  float calibrationWattS; 
  float feedingGridDirectionT;
  float calibrationWattT; 
};

struct P003GSMessage
{
  unsigned long rxTimeSec;
  float solarInputV1;
  float solarInputV2;
  float solarInputA1;
  float solarInputA2;
  float battV;
  float battCapacity;
  float battA;
  float acInputVoltageR;
  float acInputVoltageS;
  float acInputVoltageT;
  float acInputFrequency;
  float acInputCurrentR;
  float acInputCurrentS;
  float acInputCurrentT;
  float acOutputVoltageR;
  float acOutputVoltageS;
  float acOutputVoltageT;
  float acOutputFrequency;
  float acOutputCurrentR;
  float acOutputCurrentS;
  float acOutputCurrentT;
};



struct QpigsMessage
{
  unsigned long rxTimeSec;
    //(000.0 00.0 230.0 50.0 0000 0000 000 353 25.55 000 095 0039 0000 000.0 00.00 00000 00010000 00 00 00000 010^
  // 3. AC out V | 4. AC out Freq | 8. P battery voltage | 9. N battery voltage | 10. Batt Percentage |
  float gridV;
  float gridHz;
  float acOutV;
  float acOutHz;
  short acOutVa;
  short acOutW;
  byte acOutPercent;
  short busV;
  float battV;
  byte battChargeA;
  byte battPercent;
  float heatSinkDegC;
  byte solarA;
  byte solarV;
  float sccBattV;
  byte battDischargeA;
  short solarW; 

  float addSbuPriorityVersion;
  float isConfigChanged;
  float isSccFirmwareUpdated;
  
  float battVoltageToSteadyWhileCharging;
  float chargingStatus;
  float reservedY;
  float reservedZ;
  float reservedAA;
  float reservedBB;

  String rawBuffer;

  float cSOC;

}; 

struct QmodMessage
{
  char mode;
  String operationMode;
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

//QPIRI<cr>: Device Rating Information inquiry
struct QpiriMessage
{
    bool  data; //switch for the mqtt data sending
	  float gridRatingV;
    float gridRatingA;
    float acOutV;
    float gridRatingHz;
    float acOutA;
    float gridRatingW;
    float acOutRatingW;
    float battRatingV;
};
//for future use
struct QmdMessage
{
	
};

struct QidMessage
{
  char id[16];
};
//QET<cr>: Inquiry total energy
struct QetMessage
{
  short energy;
};
#endif
