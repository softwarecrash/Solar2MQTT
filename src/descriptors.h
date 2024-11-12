#pragma GCC diagnostic ignored "-Wunused-variable"
//pi_Serial flags
static const char* DESCR_req_NAK = "NAK"; 
static const char* DESCR_req_NOA = "NOA"; 
static const char* DESCR_req_ERCRC = "ERCRC"; 
 // QPIGS
static const char* DESCR_AC_In_Voltage = "AC_In_Voltage";                                   // BBB.B
static const char* DESCR_AC_In_Frequenz = "AC_In_Frequenz";                                 // CC.C
static const char* DESCR_AC_Out_Voltage = "AC_Out_Voltage";                                 // DDD.D
static const char* DESCR_AC_Out_Frequenz = "AC_Out_Frequenz";                               // EE.E
static const char* DESCR_AC_Out_VA = "AC_Out_VA";                                           // FFFF
static const char* DESCR_AC_Out_Watt = "AC_Out_Watt";                                       // GGGG
static const char* DESCR_AC_Out_Percent = "AC_Out_Percent";                                 // HHH
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
static const char* DESCR_Battery_SCC2_Volt = "Battery_SCC2_Volt";
static const char* DESCR_MPPT1_Charger_Temperature = "MPPT1_Charger_Temperature";
static const char* DESCR_MPPT2_Charger_Temperature = "MPPT2_Charger_Temperature";
static const char* DESCR_PV1_Input_Power = "PV1_Input_Power";
static const char* DESCR_PV2_Input_Power = "PV2_Input_Power";
static const char* DESCR_PV1_Input_Voltage = "PV1_Input_Voltage";
static const char* DESCR_PV2_Input_Voltage = "PV2_Input_Voltage";
static const char* DESCR_Configuration_State = "Configuration_State";  
static const char* DESCR_MPPT1_Charger_Status = "MPPT1_Charger_Status";  
static const char* DESCR_MPPT2_CHarger_Status = "MPPT2_CHarger_Status";  
static const char* DESCR_Load_Connection = "Load_Connection";
static const char* DESCR_Battery_Power_Direction = "Battery_Power_Direction";
static const char* DESCR_ACDC_Power_Direction = "ACDC_Power_Direction";
static const char* DESCR_Line_Power_Direction = "Line_Power_Direction";
static const char* DESCR_Local_Parallel_ID = "Local_Parallel_ID";
static const char* DESCR_Battery_Load = "Battery_Load";
static const char* DESCR_PV_Input_Power = "PV_Input_Power";
//QPIGS2
static const char* DESCR_PV2_Input_Current = "PV2_Input_Current";
static const char* DESCR_PV2_Charging_Power = "PV2_Charging_Power";




// QALL
//static const char* DESCR_PV_Generation_Day = "PV_Generation_Day";                           // OOOOOO
//static const char* DESCR_PV_Generation_Sum = "PV_Generation_Sum";                           // PPPPPP
//static const char* DESCR_Inverter_Operation_Mode = "Inverter_Operation_Mode";               // Q
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
//QPIWS PI30
/* static const char* DESCR_Inverter_Fault = "Inverter fault";
static const char* DESCR_Bus_Over_Fault = "Bus over fault";
static const char* DESCR_Bus_Under_Fault = "Bus under fault";
static const char* DESCR_Bus_Soft_Fail_Fault = "Bus soft fail fault";
static const char* DESCR_Line_Fail_Warning = "Line fail warning";
static const char* DESCR_OPV_Short_Warning = "OPV short warning";
static const char* DESCR_Inverter_Voltage_Too_Low_Fault = "Inverter voltage too low fault";
static const char* DESCR_Inverter_Toltage_Too_High_Fault = "Inverter voltage too high fault";
static const char* DESCR_Over_Temperature_Fault = "Over temperature fault";
static const char* DESCR_Fan_Locked_Fault = "Fan locked fault";
static const char* DESCR_Battery_Voltage_Too_High_Fault = "Battery voltage too high fault";
static const char* DESCR_Battery_Low_Alarm_Warning = "Battery low alarm warning";
static const char* DESCR_Battery_Under_Shutdown_Warning = "Battery under shutdown warning";
static const char* DESCR_Overload_Fault = "Overload fault";
static const char* DESCR_EEPROM_Fault = "EEPROM fault";
static const char* DESCR_Inverter_Over_Current_Fault = "Inverter over current fault";
static const char* DESCR_Inverter_Soft_Fail_Fault = "Inverter soft fail fault";
static const char* DESCR_Self_Test_Fail_Fault = "Self test fail fault";
static const char* DESCR_OP_DC_Voltage_Over_Fault = "OP DC voltage over fault";
static const char* DESCR_Battery_Open_Fault = "Battery open fault";
static const char* DESCR_Current_Sensor_Fail_Fault = "Current sensor fail fault";
static const char* DESCR_Battery_Short_Fault = "Battery short fault";
static const char* DESCR_Power_Limit_Warning = "Power limit warning";
static const char* DESCR_PV_Voltage_High_Warning = "PV voltage high warning";
static const char* DESCR_MPPT_Overload_Fault = "MPPT overload fault";
static const char* DESCR_MPPT_Overload_Warning = "MPPT overload warning";
static const char* DESCR_Battery_Too_Low_To_Charge_Warning = "Battery too low to charge warning"; */
//QPIGS


static const char* DESCR_AC_In_Rating_Voltage = "AC_In_Rating_Voltage";            // BBB.B
static const char* DESCR_AC_In_Rating_Current = "AC_In_Rating_Current";            // CC.C
static const char* DESCR_AC_Out_Rating_Voltage = "AC_Out_Rating_Voltage";           // DDD.D
static const char* DESCR_AC_Out_Rating_Frequency = "AC_Out_Rating_Frequency";         // EE.E
static const char* DESCR_AC_Out_Rating_Current = "AC_Out_Rating_Current";           // FF.F
static const char* DESCR_AC_Out_Rating_Apparent_Power = "AC_Out_Rating_Apparent_Power";    // HHHH
static const char* DESCR_AC_Out_Rating_Active_Power = "AC_Out_Rating_Active_Power";      // IIII
static const char* DESCR_Battery_Rating_Voltage = "Battery_Rating_Voltage";          // JJ.J
static const char* DESCR_Battery_Recharge_Voltage = "Battery_Recharge_Voltage";       // KK.K
static const char* DESCR_Battery_Under_Voltage = "Battery_Under_Voltage";           // JJ.J
static const char* DESCR_Battery_Bulk_Voltage = "Battery_Bulk_Voltage";            // KK.K
static const char* DESCR_Battery_Float_Voltage = "Battery_Float_Voltage";           // LL.L
static const char* DESCR_Battery_Type = "Battery_Type";                    // O
static const char* DESCR_Current_Max_AC_Charging_Current = "Current_Max_AC_Charging_Current"; // PP
static const char* DESCR_Current_Max_Charging_Current = "Current_Max_Charging_Current";    // QQ0
static const char* DESCR_Input_Voltage_Range = "Input_Voltage_Range";             // O
static const char* DESCR_Output_Source_Priority = "Output_Source_Priority";          // P
static const char* DESCR_Charger_Source_Priority = "Charger_Source_Priority";         // Q
static const char* DESCR_Parallel_Max_Num = "Parallel_Max_Num";                // R
static const char* DESCR_Machine_Type = "Machine_Type";                    // SS
static const char* DESCR_Topology = "Topology";                        // T
static const char* DESCR_Output_Mode = "Output_mode";                     // U
static const char* DESCR_Battery_Redischarge_Voltage = "Battery_Redischarge_Voltage";    // VV.V
static const char* DESCR_PV_OK_Condition_For_Parallel = "PV_OK_Condition_For_Parallel";    // W
static const char* DESCR_PV_Power_Balance = "PV_Power_Balance";                // X
static const char* DESCR_Max_Charging_Time_At_CV_Stage = "Max_Charging_Time_At_CV_Stage";   // YYY
static const char* DESCR_Operation_Logic = "Operation_Logic";                 // Z
static const char* DESCR_Max_Discharging_Current = "Max_Discharging_Current";         // CCC
static const char* DESCR_Solar_Power_Priority = "Solar_Power_Priority";         // Z
static const char* DESCR_MPPT_String = "MPPT_String";         // a
