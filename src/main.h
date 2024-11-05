#ifndef MAIN_H
#define MAIN_H

#include <WebSerialLite.h>
#include "descriptors.h"
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#define JSON_BUFFER 2048

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
void writeLog(const char *format, ...);

static const char *const haStaticDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {DESCR_STAT_AC_IN_RATING_CURRENT, "current-ac", "A", "current"},
    {DESCR_STAT_AC_IN_RATING_VOLTAGE, "flash-triangle-outline", "V", "voltage"},
    {DESCR_STAT_AC_OUT_RATING_ACTIVE_POWER, "sine-wave", "W", "power"},
    {DESCR_STAT_AC_OUT_RATING_APPARENT_POWER, "sine-wave", "W", "power"},
    {DESCR_STAT_AC_OUT_RATING_CURRENT, "current-ac", "A", "current"},
    {DESCR_STAT_AC_OUT_RATING_FREQUENCY, "sine-wave", "Hz", "frequency"},
    {DESCR_STAT_AC_OUT_RATING_VOLTAGE, "flash-triangle-outline", "V", "voltage"},
    {DESCR_STAT_BATTERY_BULK_VOLTAGE, "car-battery", "V", "voltage"},
    {DESCR_STAT_BATTERY_FLOAT_VOLTAGE, "car-battery", "V", "voltage"},
    {DESCR_STAT_BATTERY_RATING_VOLTAGE, "car-battery", "V", "voltage"},
    {DESCR_STAT_BATTERY_RE_CHARGE_VOLTAGE, "battery-charging-high", "V", "voltage"},
    {DESCR_STAT_BATTERY_RE_DISCHARGE_VOLTAGE, "battery-charging-outline", "V", "voltage"},
    {DESCR_STAT_BATTERY_TYPE, "car-battery", "", ""},
    {DESCR_STAT_BATTERY_UNDER_VOLTAGE, "battery-remove-outline", "V", "voltage"},
    {DESCR_STAT_CHARGER_SOURCE_PRIORITY, "ev-station", "", ""},
    {DESCR_STAT_CURRENT_MAX_AC_CHARGING_CURRENT, "current-ac", "A", "current"},
    {DESCR_STAT_CURRENT_MAX_CHARGING_CURRENT, "battery-charging", "A", "current"},
    {DESCR_STAT_DEVICE_MODEL, "battery-charging", "", ""},
    {DESCR_STAT_INPUT_VOLTAGE_RANGE, "flash-triangle-outline", "", ""},
    {DESCR_STAT_MACHINE_TYPE, "state-machine", "", ""},
    {DESCR_STAT_MAX_CHARGING_TIME_AT_CV_STAGE, "clock-time-eight-outli", "s", "duration"},
    {DESCR_STAT_MAX_DISCHARGING_CURRENT, "battery-outline", "A", "current"},
    {DESCR_STAT_MPPT_STRING, "string-lights", "", ""},
    {DESCR_STAT_OPERATION_LOGIC, "access-point", "", ""},
    {DESCR_STAT_OUTPUT_MODE, "export", "", ""},
    {DESCR_STAT_OUTPUT_SOURCE_PRIORITY, "export", "", ""},
    //{"Parallel_max_num","","",""},
    {DESCR_STAT_PROTOCOL_ID, "protocol", "", ""},
    //{"PV_OK_condition_for_parallel","solar-panel","",""},
    {DESCR_STAT_PV_POWER_BALANCE, "solar-panel", "", ""},
    {DESCR_STAT_SOLAR_POWER_PRIORITY, "priority-high", "", ""},
    {DESCR_STAT_TOPOLOGY, "earth", "", ""},
    {"Buzzer_Enabled", "tune-variant", "", ""},
    {"Overload_bypass_Enabled", "tune-variant", "", ""},
    {"Power_saving_Enabled", "tune-variant", "", ""},
    {"LCD_reset_to_default_Enabled", "tune-variant", "", ""},
    {"Overload_restart_Enabled", "tune-variant", "", ""},
    {"Over_temperature_restart_Enabled", "tune-variant", "", ""},
    {"LCD_backlight_Enabled", "tune-variant", "", ""},
    {"Primary_source_interrupt_alarm_Enabled", "tune-variant", "", ""},
    {"Record_fault_code_Enabled", "tune-variant", "", ""}};
    
static const char *const haLiveDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {DESCR_LIVE_AC_IN_FREQUENZ, "import", "Hz", "frequency"},
    {DESCR_LIVE_AC_IN_GENERATION_DAY, "import", "Wh", "energy"},
    {DESCR_LIVE_AC_IN_GENERATION_MONTH, "import", "Wh", "energy"},
    {DESCR_LIVE_AC_IN_GENERATION_SUM, "import", "Wh", "energy"},
    {DESCR_LIVE_AC_IN_GENERATION_YEAR, "import", "Wh", "energy"},
    {DESCR_LIVE_AC_IN_VOLTAGE, "import", "V", "voltage"},
    {DESCR_LIVE_AC_OUT_FREQUENZ, "export", "Hz", "frequency"},
    {DESCR_LIVE_AC_OUT_PERCENT, "export", "%", "power_factor"},
    {DESCR_LIVE_AC_OUT_VA, "export", "VA", "apparent_power"},
    {DESCR_LIVE_AC_OUT_VOLTAGE, "export", "V", "voltage"},
    {DESCR_LIVE_AC_OUT_WATT, "export", "W", "power"},
    {DESCR_LIVE_AC_OUTPUT_CURRENT, "export", "A", "current"},
    {DESCR_LIVE_AC_OUTPUT_FREQUENCY, "export", "Hz", "frequency"},
    {DESCR_LIVE_AC_OUTPUT_POWER, "export", "W", "power"},
    {DESCR_LIVE_AC_OUTPUT_VOLTAGE, "export", "V", "voltage"},
    //{"ACDC_Power_Direction","sign-direction","",""},
    {DESCR_LIVE_BATTERY_CAPACITY, "battery-high", "%", "battery"},
    //{"Battery_Charge_Current","battery-charging-high","A","current"},
    //{"Battery_Discharge_Current","battery-charging-outli","A","current"},
    {DESCR_LIVE_BATTERY_LOAD, "battery-charging-high", "A", "current"},
    {DESCR_LIVE_BATTERY_PERCENT, "battery-charging-high", "%", "battery"},
    {DESCR_LIVE_BATTERY_POWER_DIRECTION, "battery-charging-high", "", ""},
    //{"Battery_SCC_Volt","battery-high","V","voltage"},
    //{"Battery_SCC2_Volt","battery-high","V","voltage"},
    {DESCR_LIVE_BATTERY_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    {DESCR_LIVE_BATTERY_VOLTAGE, "battery-high", "V", "voltage"},
    //{"Battery_voltage_offset_fans_on","fan","",""},
    //{"Configuration_State","state-machine","",""},
    //{"Country","earth","",""},
    //{"Device_Status","state-machine","",""},
    //{"EEPROM_Version","chip","",""},
    {"Fan_speed","fan","%",""},
    {"Fault_code","alert-outline","",""},
    {DESCR_LIVE_GRID_FREQUENCY, "import", "Hz", "frequency"},
    {DESCR_LIVE_GRID_VOLTAGE, "import", "V", "voltage"},
    {DESCR_LIVE_INVERTER_BUS_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    {DESCR_LIVE_INVERTER_BUS_VOLTAGE, "flash-triangle-outline", "V", "voltage"},
    //{"Inverter_charge_state","car-turbocharger","",""},
    {DESCR_LIVE_INVERTER_OPERATION_MODE, "car-turbocharger", "", ""},
    {DESCR_LIVE_INVERTER_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    //{"Line_Power_Direction","transmission-tower","",""},
    //{"Load_Connection","connection","",""},
    {DESCR_LIVE_LOCAL_PARALLEL_ID, "card-account-details-outline", "", ""},
    //{"Max_temperature","thermometer-plus","C","temperature"},
    //{"MPPT1_Charger_Status","car-turbocharger","",""},
    {DESCR_LIVE_MPPT1_CHARGER_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    //{"MPPT2_CHarger_Status","car-turbocharger","",""},
    {DESCR_LIVE_MPPT2_CHARGER_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    {DESCR_LIVE_NEGATIVE_BATTERY_VOLTAGE, "battery-minus-outline", "V", "voltage"},
    {DESCR_LIVE_OUTPUT_CURRENT, "export", "A", "current"},
    {DESCR_LIVE_OUTPUT_LOAD_PERCENT, "export", "%", "battery"},
    {DESCR_LIVE_OUTPUT_POWER, "export", "W", "power"},
    //{"PBUS_voltage","","V","voltage"},
    {DESCR_LIVE_POSITIVE_BATTERY_VOLTAGE, "car-battery", "V", "voltage"},
    {DESCR_LIVE_PV_CHARGING_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV_GENERATION_DAY, "solar-power-variant", "Wh", "energy"},
    {DESCR_LIVE_PV_GENERATION_MONTH, "solar-power-variant", "Wh", "energy"},
    {DESCR_LIVE_PV_GENERATION_SUM, "solar-power-variant", "Wh", "energy"},
    {DESCR_LIVE_PV_GENERATION_YEAR, "solar-power-variant", "Wh", "energy"},
    {DESCR_LIVE_PV_INPUT_CURRENT, "solar-power-variant", "A", "current"},
    {DESCR_LIVE_PV_INPUT_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV_INPUT_VOLTAGE, "solar-power-variant", "V", "voltage"},
    {DESCR_LIVE_PV1_INPUT_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV1_INPUT_VOLTAGE, "solar-power-variant", "V", "voltage"},
    {DESCR_LIVE_PV2_CHARGING_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV2_INPUT_CURRENT, "solar-power-variant", "A", "current"},
    {DESCR_LIVE_PV2_INPUT_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV2_INPUT_VOLTAGE, "solar-power-variant", "V", "voltage"},
    {DESCR_LIVE_PV3_INPUT_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_PV3_INPUT_VOLTAGE, "solar-power-variant", "V", "voltage"},
    //{"SBUS_voltage","flash-triangle-outline","V","voltage"},
    {DESCR_LIVE_SOLAR_FEED_TO_GRID_POWER, "solar-power-variant", "W", "power"},
    {DESCR_LIVE_SOLAR_FEED_TO_GRID_STATUS, "solar-power-variant", "", ""},
    //{"Status_Flag","flag","",""},
    //{"Time_until_absorb_charge","solar-power-variant","s","duration"},
    //{"Time_until_float_charge","solar-power-variant","s","duration"},
    {DESCR_LIVE_TRACKER_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    {DESCR_LIVE_TRANSFORMER_TEMPERATURE, "thermometer-lines", "°C", "temperature"},
    {DESCR_LIVE_WARNING_CODE, "alert-outline", "", "" }};
#endif