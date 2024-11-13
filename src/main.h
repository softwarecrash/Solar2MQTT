#ifndef MAIN_H
#define MAIN_H

#include <WebSerialLite.h>
#include "descriptors.h"
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
//#define ARDUINOJSON_ENABLE_PROGMEM 1

#ifdef isUART_HARDWARE
#define INVERTER_TX 1
#define INVERTER_RX 3
#define LED_COM 5
#define LED_SRV 0
#define LED_NET 4
#else
#define INVERTER_TX 13
#define INVERTER_RX 12
#define TEMPSENS_PIN 4 // DS18B20 Pin; D2 on Wemos D1 Mini
#endif

#define LED_PIN 02 // D4 with the LED on Wemos D1 Mini

#define DBG_BAUD 115200
#define DBG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#define DBG_WEB(...) WebSerial.print(__VA_ARGS__)
#define SOFTWARE_VERSION SWVERSION
#define DBG Serial
#define DBG_BEGIN(...) DBG.begin(__VA_ARGS__)
#define DBG_PRINTLN(...) DBG.println(__VA_ARGS__)

typedef enum
{
    NoD,
    PI18,
    PI30,
    MODBUS_MUST,
    MODBUS_DEYE,
    MODBUS_ANENJI
} protocol_type_t;

/**
 * @brief callback function for wifimanager save config data
 *
 */
void saveConfigCallback();

/**
 * @brief callback function for data
 *
 */
bool prozessData();

/**
 * @brief fires up the websocket and send data to the clients
 *
 */
void notifyClients();

/**
 * @brief build the topic string and return
 *
 */
char *topicBuilder(char *buffer, char const *path, char const *numering);

/**
 * @brief mqtt connect function, check if connection etablished and reconnect and subscribe to spezific topics if needed
 *
 */
bool connectMQTT();

/**
 * @brief send the data to mqtt
 *
 */
bool sendtoMQTT();

/**
 * @brief get the basic device data
 *
 */
// void getJsonDevice();

/**
 * @brief read the data from bms and put it in the json
 */
void getJsonData();

/**
 * @brief callback function, watch the sunscribed topics and process the data
 *
 */
void mqttcallback(char *top, unsigned char *payload, unsigned int length);

bool sendHaDiscovery();

/**
 * @brief this function act like s/n/printf() and give the output to the configured serial and webserial
 *
 */
void writeLog(const char* format, ...);

static const char* DESCR_current_ac = "current-ac";
static const char* DESCR_current = "current";
static const char* DESCR_voltage = "voltage";
static const char* DESCR_power = "power";
static const char* DESCR_energy = "energy";
static const char* DESCR_frequency = "frequency";
static const char* DESCR_A = "A";
static const char* DESCR_W = "W";
static const char* DESCR_V = "V";
static const char* DESCR_Hz = "Hz";
static const char* DESCR_Wh = "Wh";
static const char* DESCR_flash_triangle_outline = "flash-triangle-outline";
static const char* DESCR_sine_wave = "sine-wave";
static const char* DESCR_car_battery = "car-battery";
static const char* DESCR_thermometer_lines = "thermometer-lines";
static const char* DESCR_import = "import";
static const char* DESCR_export = "export";

//static const char* DESCR_ = "";

static const char* DESCR_solar_power_variant = "solar-power-variant";

static const char *const haStaticDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {DESCR_AC_In_Rating_Current, DESCR_current_ac, DESCR_A, DESCR_current},
    {DESCR_AC_In_Rating_Voltage, DESCR_flash_triangle_outline, DESCR_V, DESCR_voltage},
    {DESCR_AC_Out_Rating_Active_Power, DESCR_sine_wave, DESCR_W, DESCR_power},
    {DESCR_AC_Out_Rating_Apparent_Power, DESCR_sine_wave, DESCR_W, DESCR_power},
    {DESCR_AC_Out_Rating_Current, DESCR_current_ac, DESCR_A, DESCR_current},
    {DESCR_AC_Out_Rating_Frequency, DESCR_sine_wave, DESCR_Hz, DESCR_frequency},
    {DESCR_AC_Out_Rating_Voltage, DESCR_flash_triangle_outline, DESCR_V, DESCR_voltage},
    {DESCR_Battery_Bulk_Voltage, DESCR_car_battery, DESCR_V, DESCR_voltage},
    {DESCR_Battery_Float_Voltage, DESCR_car_battery, DESCR_V, DESCR_voltage},
    {DESCR_Battery_Rating_Voltage, DESCR_car_battery, DESCR_V, DESCR_voltage},
    {DESCR_Battery_Recharge_Voltage, "battery-charging-high", DESCR_V, DESCR_voltage},
    {DESCR_Battery_Redischarge_Voltage, "battery-charging-outline", DESCR_V, DESCR_voltage},
    {DESCR_Battery_Type, DESCR_car_battery, "", ""},
    {DESCR_Battery_Under_Voltage, "battery-remove-outline", DESCR_V, DESCR_voltage},
    {DESCR_Charger_Source_Priority, "ev-station", "", ""},
    {DESCR_Current_Max_AC_Charging_Current, DESCR_current_ac, DESCR_A, DESCR_current},
    {DESCR_Current_Max_Charging_Current, "battery-charging", DESCR_A, DESCR_current},
    {DESCR_Device_Model, "battery-charging", "", ""},
    {DESCR_Input_Voltage_Range, DESCR_flash_triangle_outline, "", ""},
    {DESCR_Machine_Type, "state-machine", "", ""},
    {DESCR_Max_Charging_Time_At_CV_Stage, "clock-time-eight-outli", "s", "duration"},
    {DESCR_Max_Discharging_Current, "battery-outline", DESCR_A, DESCR_current},
    {DESCR_MPPT_String, "string-lights", "", ""},
    {DESCR_Operation_Logic, "access-point", "", ""},
    {DESCR_Output_Mode, DESCR_export, "", ""},
    {DESCR_Output_Source_Priority, DESCR_export, "", ""},
    {DESCR_Parallel_Max_Num,"","",""},
    {DESCR_Protocol_ID, "protocol", "", ""},
    {DESCR_PV_OK_Condition_For_Parallel,"solar-panel","",""},
    {DESCR_PV_Power_Balance, "solar-panel", "", ""},
    {DESCR_Solar_Power_Priority, "priority-high", "", ""},
    {DESCR_Topology, "earth", "", ""},
    {DESCR_Buzzer_Enabled, "tune-variant", "", ""},
    {DESCR_Overload_Bypass_Enabled, "tune-variant", "", ""},
    {DESCR_Power_Saving_Enabled, "tune-variant", "", ""},
    {DESCR_LCD_Reset_To_Default_Enabled, "tune-variant", "", ""},
    {DESCR_Overload_Restart_Enabled, "tune-variant", "", ""},
    {DESCR_Over_Temperature_Restart_Enabled, "tune-variant", "", ""},
    {DESCR_LCD_Backlight_Enabled, "tune-variant", "", ""},
    {DESCR_Primary_Source_Interrupt_Alarm_Enabled, "tune-variant", "", ""},
    {DESCR_Record_Fault_Code_Enabled, "tune-variant", "", ""}};
static const char *const haLiveDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {DESCR_AC_In_Frequenz, DESCR_import, DESCR_Hz, DESCR_frequency},
    {DESCR_AC_In_Generation_Day, DESCR_import, DESCR_Wh, DESCR_energy},
    {DESCR_AC_In_Generation_Month, DESCR_import, DESCR_Wh, DESCR_energy},
    {DESCR_AC_In_Generation_Sum, DESCR_import, DESCR_Wh, DESCR_energy},
    {DESCR_AC_In_Generation_Year, DESCR_import, DESCR_Wh, DESCR_energy},
    {DESCR_AC_In_Voltage, DESCR_import, DESCR_V, DESCR_voltage},
    {DESCR_AC_Out_Frequenz, DESCR_export, DESCR_Hz, DESCR_frequency},
    {DESCR_AC_Out_Percent, DESCR_export, "%", "power_factor"},
    {DESCR_AC_Out_VA, DESCR_export, "VA", "apparent_power"},
    {DESCR_AC_Out_Voltage, DESCR_export, DESCR_V, DESCR_voltage},
    {DESCR_AC_Out_Watt, DESCR_export, DESCR_W, DESCR_power},

    {DESCR_AC_output_current, DESCR_export, DESCR_A, DESCR_current},
    {DESCR_AC_output_frequency, DESCR_export, DESCR_Hz, DESCR_frequency},
    {DESCR_AC_output_power, DESCR_export, DESCR_W, DESCR_power},
    {DESCR_AC_output_voltage, DESCR_export, DESCR_V, DESCR_voltage},
    //{"ACDC_Power_Direction","sign-direction","",""},
    {"Battery_capacity", "battery-high", "%", "battery"},
    //{"Battery_Charge_Current","battery-charging-high",DESCR_A,DESCR_current},
    //{"Battery_Discharge_Current","battery-charging-outli",DESCR_A,DESCR_current},
    {DESCR_Battery_Load, "battery-charging-high", DESCR_A, DESCR_current},
    {DESCR_Battery_Percent, "battery-charging-high", "%", "battery"},
    {DESCR_Battery_Power_Direction, "battery-charging-high", "", ""},
    //{"Battery_SCC_Volt","battery-high",DESCR_V,DESCR_voltage},
    //{"Battery_SCC2_Volt","battery-high",DESCR_V,DESCR_voltage},
    {DESCR_Battery_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    {DESCR_Battery_Voltage, "battery-high", DESCR_V, DESCR_voltage},
    //{"Battery_voltage_offset_fans_on","fan","",""},
    //{"Configuration_State","state-machine","",""},
    //{"Country","earth","",""},
    //{"Device_Status","state-machine","",""},
    //{"EEPROM_Version","chip","",""},
    {DESCR_Fan_Speed,"fan","%",""},
    {DESCR_Fault_Code,"alert-outline","",""},
    {"Grid_frequency", DESCR_import, DESCR_Hz, DESCR_frequency},
    {"Grid_voltage", DESCR_import, DESCR_V, DESCR_voltage},
    {DESCR_Inverter_Bus_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    {DESCR_Inverter_Bus_Voltage, DESCR_flash_triangle_outline, DESCR_V, DESCR_voltage},
    //{"Inverter_charge_state","car-turbocharger","",""},
    {DESCR_Inverter_Operation_Mode, "car-turbocharger", "", ""},
    {DESCR_Inverter_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    //{"Line_Power_Direction","transmission-tower","",""},
    //{"Load_Connection","connection","",""},
    {DESCR_Local_Parallel_ID, "card-account-details-outline", "", ""},
    //{"Max_temperature","thermometer-plus","C","temperature"},
    //{"MPPT1_Charger_Status","car-turbocharger","",""},
    {DESCR_MPPT1_Charger_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    //{"MPPT2_CHarger_Status","car-turbocharger","",""},
    {DESCR_MPPT2_Charger_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    {DESCR_Negative_battery_voltage, "battery-minus-outline", DESCR_V, DESCR_voltage},
    {DESCR_Output_current, DESCR_export, DESCR_A, DESCR_current},
    {DESCR_Output_load_percent, DESCR_export, "%", "battery"},
    {DESCR_Output_power, DESCR_export, DESCR_W, DESCR_power},
    //{"PBUS_voltage","",DESCR_V,DESCR_voltage},
    {DESCR_Positive_battery_voltage, DESCR_car_battery, DESCR_V, DESCR_voltage},
    {DESCR_PV_Charging_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_PV_Generation_Day, DESCR_solar_power_variant, DESCR_Wh, DESCR_energy},
    {DESCR_PV_Generation_Month, DESCR_solar_power_variant, DESCR_Wh, DESCR_energy},
    {DESCR_PV_Generation_Sum, DESCR_solar_power_variant, DESCR_Wh, DESCR_energy},
    {DESCR_PV_Generation_Year, DESCR_solar_power_variant, DESCR_Wh, DESCR_energy},
    {DESCR_PV_Input_Current, DESCR_solar_power_variant, DESCR_A, DESCR_current},
    {DESCR_PV_Input_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_PV_Input_Voltage, DESCR_solar_power_variant, DESCR_V, DESCR_voltage},
    {DESCR_PV1_Input_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_PV1_Input_Voltage, DESCR_solar_power_variant, DESCR_V, DESCR_voltage},
    {DESCR_PV2_Charging_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_PV2_Input_Current, DESCR_solar_power_variant, DESCR_A, DESCR_current},
    {DESCR_PV2_Input_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_PV2_Input_Voltage, DESCR_solar_power_variant, DESCR_V, DESCR_voltage},
    {"PV3_Input_Power", DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {"PV3_Input_Voltage", DESCR_solar_power_variant, DESCR_V, DESCR_voltage},
    //{"SBUS_voltage",DESCR_flash_triangle_outline,DESCR_V,DESCR_voltage},
    {DESCR_Solar_Feed_To_Grid_Power, DESCR_solar_power_variant, DESCR_W, DESCR_power},
    {DESCR_Solar_Feed_To_Grid_Status, DESCR_solar_power_variant, "", ""},
    {DESCR_Status_Flag,"flag","",""},
    {DESCR_Time_Until_Absorb_Charge,DESCR_solar_power_variant,"s","duration"},
    {DESCR_Time_Until_Float_Charge,DESCR_solar_power_variant,"s","duration"},
    {DESCR_Tracker_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    {DESCR_Transformer_Temperature, DESCR_thermometer_lines, "°C", "temperature"},
    {DESCR_Warning_Code, "alert-outline", "", ""}};

    #endif