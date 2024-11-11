/* // QPIGS
#define DESCR_AC_in_Voltage  "AC_in_Voltage"                                   // BBB.B
#define DESCR_AC_in_Frequenz  "AC_in_Frequenz"                                 // CC.C
#define DESCR_AC_out_Voltage  "AC_out_Voltage"                                 // DDD.D
#define DESCR_AC_out_Frequenz  "AC_out_Frequenz"                               // EE.E
#define DESCR_AC_out_VA  "AC_out_VA"                                           // FFFF
#define DESCR_AC_out_Watt  "AC_out_Watt"                                       // GGGG
#define DESCR_AC_out_Percent  "AC_out_Percent"                                 // HHH
#define DESCR_Inverter_Bus_Voltage  "Inverter_Bus_Voltage"                     // III
#define DESCR_Battery_Voltage  "Battery_Voltage"                               // JJ.JJ
#define DESCR_Battery_Charge_Current  "Battery_Charge_Current"                 // KKK
#define DESCR_Battery_Percent  "Battery_Percent"                               // OOO
#define DESCR_Inverter_Bus_Temperature  "Inverter_Bus_Temperature"             // TTTT
#define DESCR_PV_Input_Current  "PV_Input_Current"                             // EE.E
#define DESCR_PV_Input_Voltage  "PV_Input_Voltage"                             // UUU.U
#define DESCR_Battery_SCC_Volt  "Battery_SCC_Volt"                             // WW.WW
#define DESCR_Battery_Discharge_Current  "Battery_Discharge_Current"           // PPPP
#define DESCR_Status_Flag  "Status_Flag"                                       // b0-b7
#define DESCR_Battery_Voltage_Offset_Fans_On  "Battery_Voltage_Offset_Fans_On" // QQ
#define DESCR_EEPROM_Version  "EEPROM_Version"                                 // VV
#define DESCR_PV_Charging_Power  "PV_Charging_Power"                           // MMMM
#define DESCR_Device_Status  "Device_Status"                                   // b8-b10
#define DESCR_Solar_Feed_To_Grid_Status  "Solar_Feed_To_Grid_Status"           // Y
#define DESCR_Country  "Country"                                               // ZZ
#define DESCR_Solar_Feed_To_Grid_Power  "Solar_Feed_To_Grid_Power"             // AAAA
// QALL
#define DESCR_PV_Generation_Day  "PV_Generation_Day"                           // OOOOOO
#define DESCR_PV_Generation_Sum  "PV_Generation_Sum"                           // PPPPPP
#define DESCR_Inverter_Operation_Mode  "Inverter_Operation_Mode"               // Q
#define DESCR_Warning_Code  "Warning_Code"                                     // KK
#define DESCR_Fault_Code  "Fault_Code"                                         // SS */



/* 
 // QPIGS static const char HTML_CONFIRM_RESET
const char *const DESCR_AC_in_Voltage = "AC_in_Voltage";                                   // BBB.B
const char *const DESCR_AC_in_Frequenz = "AC_in_Frequenz";                                 // CC.C
const char *const DESCR_AC_out_Voltage = "AC_out_Voltage";                                 // DDD.D
const char *const DESCR_AC_out_Frequenz = "AC_out_Frequenz";                               // EE.E
const char *const DESCR_AC_out_VA = "AC_out_VA";                                           // FFFF
const char *const DESCR_AC_out_Watt = "AC_out_Watt";                                       // GGGG
const char *const DESCR_AC_out_Percent = "AC_out_Percent";                                 // HHH
const char *const DESCR_Inverter_Bus_Voltage = "Inverter_Bus_Voltage";                     // III
const char *const DESCR_Battery_Voltage = "Battery_Voltage";                               // JJ.JJ
const char *const DESCR_Battery_Charge_Current = "Battery_Charge_Current";                 // KKK
const char *const DESCR_Battery_Percent = "Battery_Percent";                               // OOO
const char *const DESCR_Inverter_Bus_Temperature = "Inverter_Bus_Temperature";             // TTTT
const char *const DESCR_PV_Input_Current = "PV_Input_Current";                             // EE.E
const char *const DESCR_PV_Input_Voltage = "PV_Input_Voltage";                             // UUU.U
const char *const DESCR_Battery_SCC_Volt = "Battery_SCC_Volt";                             // WW.WW
const char *const DESCR_Battery_Discharge_Current = "Battery_Discharge_Current";           // PPPP
const char *const DESCR_Status_Flag = "Status_Flag";                                       // b0-b7
const char *const DESCR_Battery_Voltage_Offset_Fans_On = "Battery_Voltage_Offset_Fans_On"; // QQ
const char *const DESCR_EEPROM_Version = "EEPROM_Version";                                 // VV
const char *const DESCR_PV_Charging_Power = "PV_Charging_Power";                           // MMMM
const char *const DESCR_Device_Status = "Device_Status";                                   // b8-b10
const char *const DESCR_Solar_Feed_To_Grid_Status = "Solar_Feed_To_Grid_Status";           // Y
const char *const DESCR_Country = "Country";                                               // ZZ
const char *const DESCR_Solar_Feed_To_Grid_Power = "Solar_Feed_To_Grid_Power";             // AAAA
// QALL
const char *const DESCR_PV_Generation_Day = "PV_Generation_Day";                           // OOOOOO
const char *const DESCR_PV_Generation_Sum = "PV_Generation_Sum";                           // PPPPPP
const char *const DESCR_Inverter_Operation_Mode = "Inverter_Operation_Mode";               // Q
const char *const DESCR_Warning_Code = "Warning_Code";                                     // KK
const char *const DESCR_Fault_Code = "Fault_Code";                                         // SS 
//Q1
const char *const DESCR_Time_Until_Absorb_Charge = "Time_Until_Absorb_Charge"; // time since absorb start
const char *const DESCR_Time_Until_Float_Charge = "Time_Until_Float_Charge";   // tiem since float charge start
const char *const DESCR_SCC_Flag = "SCC_Flag";                                 // SCC Flag
const char *const DESCR_AllowSccOnFlag = "AllowSccOnFlag";                     // AllowSccOnFlag
const char *const DESCR_Charge_Average_Current = "Charge_Average_Current";     // ChargeAverageCurrent
const char *const DESCR_Tracker_Temperature = "Tracker_Temperature";           // Temp sensor 1 SCC PWM temperature
const char *const DESCR_Inverter_Temperature = "Inverter_Temperature";         // temp sensor 2
const char *const DESCR_Battery_Temperature = "Battery_Temperature";           // temp sensor 3
const char *const DESCR_Transformer_Temperature = "Transformer_Temperature";   //
const char *const DESCR_Fan_Lock_Status = "Fan_Lock_Status";                   // error flag for blocked fan
const char *const DESCR_Fan_Speed = "Fan_Speed";                               // Fan PWM speed
const char *const DESCR_SCC_Charge_Power = "SCC_Charge_Power";                 // SCC charge power
const char *const DESCR_Parallel_Warning = "Parallel_Warning";                 // Parallel Warning
const char *const DESCR_Sync_Frequency = "Sync_Frequency";                     // Sync frequency
const char *const DESCR_Inverter_Charge_State = "Inverter_Charge_State";       // charge state
const char *const DESCR_unknown = "";                                          // unknown state */



//pi_Serial flags
static const char* DESCR_req_NAK = "NAK"; 
static const char* DESCR_req_NOA = "NOA"; 
static const char* DESCR_req_ERCRC = "ERCRC"; 
 // QPIGS static const char* HTML_CONFIRM_RESET
static const char* DESCR_AC_in_Voltage = "AC_in_Voltage";                                   // BBB.B
static const char* DESCR_AC_in_Frequenz = "AC_in_Frequenz";                                 // CC.C
static const char* DESCR_AC_out_Voltage = "AC_out_Voltage";                                 // DDD.D
static const char* DESCR_AC_out_Frequenz = "AC_out_Frequenz";                               // EE.E
static const char* DESCR_AC_out_VA = "AC_out_VA";                                           // FFFF
static const char* DESCR_AC_out_Watt = "AC_out_Watt";                                       // GGGG
static const char* DESCR_AC_out_Percent = "AC_out_Percent";                                 // HHH
static const char* DESCR_Inverter_Bus_Voltage = "Inverter_Bus_Voltage";                     // III
static const char* DESCR_Battery_Voltage = "Battery_Voltage";                               // JJ.JJ
static const char* DESCR_Battery_Charge_Current = "Battery_Charge_Current";                 // KKK
static const char* DESCR_Battery_Percent = "Battery_Percent";                               // OOO
static const char* DESCR_Inverter_Bus_Temperature = "Inverter_Bus_Temperature";             // TTTT
static const char* DESCR_PV_Input_Current = "PV_Input_Current";                             // EE.E
static const char* DESCR_PV_Input_Voltage = "PV_Input_Voltage";                             // UUU.U
static const char* DESCR_Battery_SCC_Volt = "Battery_SCC_Volt";                             // WW.WW
static const char* DESCR_Battery_Discharge_Current = "Battery_Dischar*ge_Current";           // PPPP
static const char* DESCR_Status_Flag = "Status_Flag";                                       // b0-b7
static const char* DESCR_Battery_Voltage_Offset_Fans_On = "Battery_Voltage_Offset_Fans_On"; // QQ
static const char* DESCR_EEPROM_Version = "EEPROM_Version";                                 // VV
static const char* DESCR_PV_Charging_Power = "PV_Charging_Power";                           // MMMM
static const char* DESCR_Device_Status = "Device_Status";                                   // b8-b10
static const char* DESCR_Solar_Feed_To_Grid_Status = "Solar_Feed_To_Grid_Status";           // Y
static const char* DESCR_Country = "Country";                                               // ZZ
static const char* DESCR_Solar_Feed_To_Grid_Power = "Solar_Feed_To_Grid_Power";             // AAAA
// QALL
static const char* DESCR_PV_Generation_Day = "PV_Generation_Day";                           // OOOOOO
static const char* DESCR_PV_Generation_Sum = "PV_Generation_Sum";                           // PPPPPP
static const char* DESCR_Inverter_Operation_Mode = "Inverter_Operation_Mode";               // Q
static const char* DESCR_Warning_Code = "Warning_Code";                                     // KK
static const char* DESCR_Fault_Code = "Fault_Code";                                         // SS 
//Q1
static const char* DESCR_Time_Until_Absorb_Charge = "Time_Until_Absorb_Charge"; // time since absorb start
static const char* DESCR_Time_Until_Float_Charge = "Time_Until_Float_Charge";   // tiem since float char*ge start
static const char* DESCR_SCC_Flag = "SCC_Flag";                                 // SCC Flag
static const char* DESCR_AllowSccOnFlag = "AllowSccOnFlag";                     // AllowSccOnFlag
static const char* DESCR_Charge_Average_Current = "Charge_Average_Current";     // ChargeAverageCurrent
static const char* DESCR_Tracker_Temperature = "Tracker_Temperature";           // Temp sensor 1 SCC PWM temperature
static const char* DESCR_Inverter_Temperature = "Inverter_Temperature";         // temp sensor 2
static const char* DESCR_Battery_Temperature = "Battery_Temperature";           // temp sensor 3
static const char* DESCR_Transformer_Temperature = "Transformer_Temperature";   //
static const char* DESCR_Fan_Lock_Status = "Fan_Lock_Status";                   // error flag for blocked fan
static const char* DESCR_Fan_Speed = "Fan_Speed";                               // Fan PWM speed
static const char* DESCR_SCC_Charge_Power = "SCC_Charge_Power";                 // SCC char*ge power
static const char* DESCR_Parallel_Warning = "Parallel_Warning";                 // Parallel Warning
static const char* DESCR_Sync_Frequency = "Sync_Frequency";                     // Sync frequency
static const char* DESCR_Inverter_Charge_State = "Inverter_Charge_State";       // charge state
static const char* DESCR_unknown = "";                                          // unknown state
//Inverter_Charge_State states
static const char* DESCR_No_Charging ="No charging";
static const char* DESCR_Bulk_Stage = "Bulk stage";
static const char* DESCR_Absorb = "Absorb";
static const char* DESCR_Float = "Float";
//QPI
static const char* DESCR_Protocol_ID = "Protocol_ID";
//QEX
static const char* DESCR_PV_Generation_Sum = "PV_Generation_Sum";
static const char* DESCR_PV_Generation_Year = "PV_Generation_Year";
static const char* DESCR_PV_Generation_Month = "PV_Generation_Month";
static const char* DESCR_PV_Generation_Day = "PV_Generation_Day";
static const char* DESCR_AC_In_Generation_Sum = "AC_In_Generation_Sum";
static const char* DESCR_AC_In_Generation_Year = "AC_In_Generation_Year";
static const char* DESCR_AC_In_Generation_Month = "AC_In_Generation_Month";
static const char* DESCR_AC_In_Generation_Day = "AC_In_Generation_Day";
//QFLAG
static const char* DESCR_Buzzer_Enabled = "Buzzer_Enabled";
static const char* DESCR_Overload_Bypass_Enabled = "Overload_Bypass_Enabled";
static const char* DESCR_Power_Saving_Enabled = "Power_Saving_Enabled";
static const char* DESCR_LCD_Reset_To_Default_Enabled = "LCD_Reset_To_Default_Enabled";
static const char* DESCR_Overload_Restart_Enabled = "Overload_Restart_Enabled";
static const char* DESCR_Over_Temperature_Restart_Enabled = "Over_Temperature_Restart_Enabled";
static const char* DESCR_LCD_Backlight_Enabled = "LCD_Backlight_Enabled";
static const char* DESCR_Primary_Source_Interrupt_Alarm_Enabled = "Primary_Source_Interrupt_Alarm_Enabled";
static const char* DESCR_Record_Fault_Code_Enabled = "Record_Fault_Code_Enabled";
static const char* DESCR_ = "";
//QMN
static const char* DESCR_Device_Model = "Device_Model";
//QMOD
static const char* DESCR_Inverter_Operation_Mode = "Inverter_Operation_Mode";
static const char* DESCR_Power_On = "Power On";
static const char* DESCR_Standby = "Standby";
static const char* DESCR_Bypass = "Bypass";
static const char* DESCR_Battery = "Battery";
static const char* DESCR_Fault = "Fault";
static const char* DESCR_Hybrid = "Hybrid";
static const char* DESCR_No_Data = "No Data";
