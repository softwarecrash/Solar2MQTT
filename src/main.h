#include <WebSerialLite.h>
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#define JSON_BUFFER 4096
#define MQTT_BUFFER 512
#define DEBUG_BAUD 115200

#ifdef isUART_HARDWARE
#define INVERTER_TX 1
#define INVERTER_RX 3
#else
#define INVERTER_TX 13
#define INVERTER_RX 12
#endif

#define LED_PIN 02 //D4 with the LED on Wemos D1 Mini

#define DEBUG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#define DEBUG_WEBF(...) WebSerial.printf(__VA_ARGS__)
#define DEBUG_WEB(...) WebSerial.print(__VA_ARGS__)

#ifdef isDEBUG
#define SOFTWARE_VERSION SWVERSION " " HWBOARD " " __DATE__ " " __TIME__
#define DEBUG Serial
#define DEBUG_BEGIN(...) DEBUG.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) DEBUG.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) DEBUG.printf(__VA_ARGS__)
#define DEBUG_WRITE(...) DEBUG.write(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DEBUG.println(__VA_ARGS__)
#else
#define SOFTWARE_VERSION SWVERSION
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTF(...)
#define DEBUG_WRITE(...)
#define DEBUG_PRINTLN(...)
#endif

/**
 * @brief callback function for wifimanager save config data
 * 
 */
void saveConfigCallback();

/**
 * @brief callback function for data
 * 
 */
void prozessData();

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
//void getJsonDevice();

/**
 * @brief read the data from bms and put it in the json
 */
void getJsonData();

/**
 * @brief callback function, watch the sunscribed topics and process the data
 * 
 */
void mqttcallback(char *top, unsigned char *payload, unsigned int length);

/**
 * @brief 
 * 
 */
void prozessData();

bool  sendHaDiscovery();
static const char *const haStaticDescriptor[][4]{
      // state_topic, icon, unit_ofmeasurement, class
{"AC_in_rating_current","mdi:current-ac","A","current"},
{"AC_in_rating_voltage","mdi:flash-triangle-outline","V","voltage"},
{"AC_out_rating_active_power","mdi:sine-wave","W","power"},
{"AC_out_rating_apparent_power","mdi:sine-wave","W","power"},
{"AC_out_rating_current","mdi:current-ac","A","current"},
{"AC_out_rating_frequency","mdi:sine-wave","Hz","frequency"},
{"AC_out_rating_voltage","mdi:flash-triangle-outline","V","voltage"},
{"Battery_bulk_voltage","mdi:car-battery","V","voltage"},
{"Battery_float_voltage","mdi:car-battery","V","voltage"},
{"Battery_rating_voltage","mdi:car-battery","V","voltage"},
{"Battery_re-charge_voltage","mdi:battery-charging-high","V","voltage"},
{"Battery_re-discharge_voltage","mdi:battery-charging-outli","V","voltage"},
{"Battery_type","mdi:car-battery","",""},
{"Battery_under_voltage","mdi:battery-remove-outline","V","voltage"},
{"Charger_source_priority","mdi:ev-station","",""},
{"Current_max_AC_charging_current","mdi:current-ac","A","current"},
{"Current_max_charging_current","mdi:battery-charging","A","current"},
{"Device_Model","mdi:battery-charging","",""},
{"Input_voltage_range","mdi:flash-triangle-outline","V","voltage"},
{"Machine_type","mdi:state-machine","",""},
{"Max_charging_time_at_CV_stage","mdi:clock-time-eight-outli","s","duration"},
{"Max_discharging_current","mdi:battery-outline","A","current"},
{"MPPT_string","mdi:string-lights","",""},
{"Operation_Logic","","",""},
{"Output_mode","mdi:export","",""},
{"Output_source_priority","mdi:export","",""},
//{"Parallel_max_num","","",""},
{"Protocol_ID","","",""},
//{"PV_OK_condition_for_parallel","mdi:solar-panel","",""},
{"PV_power_balance","mdi:solar-panel","",""},
{"Solar_power_priority","","",""},
{"Topology","","",""}
      };
static const char *const haLiveDescriptor[][4]{
      // state_topic, icon, unit_ofmeasurement, class
{"AC_in_Frequenz","mdi:import","Hz","frequency"},
{"AC_in_generation_day","mdi:import","Wh","energy"},
{"AC_in_generation_month","mdi:import","Wh","energy"},
{"AC_in_generation_sum","mdi:import","Wh","energy"},
{"AC_in_generation_year","mdi:import","Wh","energy"},
{"AC_in_Voltage","mdi:import","V","voltage"},
{"AC_out_Frequenz","mdi:export","Hz","frequency"},
{"AC_out_percent","mdi:export","%","powerfactor"},
{"AC_out_VA","mdi:export","VA","apparent_power"},
{"AC_out_Voltage","mdi:export","V","voltage"},
{"AC_out_Watt","mdi:export","W","power"},
{"AC_output_current","mdi:export","A","current"},
{"AC_output_frequency","mdi:export","Hz","frequency"},
{"AC_output_power","mdi:export","W","power"},
{"AC_output_voltage","mdi:export","V","voltage"},
//{"ACDC_Power_Direction","","",""},
{"Battery_capacity","mdi:battery-high","%","battery"},
//{"Battery_Charge_Current","mdi:battery-charging-high","A","current"},
//{"Battery_Discharge_Current","mdi:battery-charging-outli","A","current"},
{"Battery_Load","mdi:battery-charging-high","A","current"},
{"Battery_Percent","mdi:battery-charging-high","%","battery"},
{"Battery_Power_Direction","mdi:battery-charging-high","",""},
//{"Battery_SCC_Volt","mdi:battery-high","V","voltage"},
//{"Battery_SCC2_Volt","mdi:battery-high","V","voltage"},
{"Battery_temperature","mdi:thermometer-lines","C","temperature"},
{"Battery_Voltage","mdi:battery-high","V","voltage"},
//{"Battery_voltage_offset_fans_on","mdi:fan","",""},
//{"Configuration_State","","",""},
//{"Country","mdi:earth","",""},
//{"Device_Status","","",""},
//{"EEPROM_Version","","",""},
{"Fan_speed","","",""},
//{"Fault_code","","",""},
{"Grid_frequency","","Hz","frequency"},
{"Grid_voltage","","V","voltage"},
{"Inverter_Bus_Temperature","mdi:thermometer-lines","C","temperature"},
{"Inverter_Bus_Voltage","","V","voltage"},
//{"Inverter_charge_state","","",""},
{"Inverter_Operation_Mode","","",""},
{"Inverter_temperature","mdi:thermometer-lines","C","temperature"},
//{"Line_Power_Direction","","",""},
//{"Load_Connection","","",""},
{"Local_Parallel_ID","","",""},
//{"Max_temperature","","C","temperature"},
//{"MPPT1_Charger_Status","","",""},
{"MPPT1_Charger_Temperature","mdi:thermometer-lines","C","temperature"},
//{"MPPT2_CHarger_Status","","",""},
{"MPPT2_Charger_Temperature","mdi:thermometer-lines","C","temperature"},
{"Negative_battery_voltage","mdi:battery-minus-outline","V","voltage"},
{"Output_current","mdi:export","A","current"},
{"Output_load_percent","mdi:export","%",""},
{"Output_power","mdi:export","W","power"},
//{"PBUS_voltage","","V","voltage"},
{"Positive_battery_voltage","mdi:car-battery","V","voltage"},
{"PV_Charging_Power","mdi:solar-power-variant","W","power"},
{"PV_generation_day","mdi:solar-power-variant","Wh","energy"},
{"PV_generation_month","mdi:solar-power-variant","Wh","energy"},
{"PV_generation_sum","mdi:solar-power-variant","Wh","energy"},
{"PV_generation_year","mdi:solar-power-variant","Wh","energy"},
{"PV_Input_Current","mdi:solar-power-variant","A","current"},
{"PV_Input_Power","mdi:solar-power-variant","W","power"},
{"PV_Input_Voltage","mdi:solar-power-variant","V","voltage"},
{"PV1_input_power","mdi:solar-power-variant","W","power"},
{"PV1_input_voltage","mdi:solar-power-variant","V","voltage"},
{"PV2_Charging_Power","mdi:solar-power-variant","W","power"},
{"PV2_Input_Current","mdi:solar-power-variant","A","current"},
{"PV2_input_power","mdi:solar-power-variant","W","power"},
{"PV2_input_voltage","mdi:solar-power-variant","V","voltage"},
{"PV3_input_power","mdi:solar-power-variant","W","power"},
{"PV3_input_voltage","mdi:solar-power-variant","V","voltage"},
//{"SBUS_voltage","","V","voltage"},
{"Solar_feed_to_grid_power","mdi:solar-power-variant","W","power"},
{"Solar_feed_to_Grid_status","mdi:solar-power-variant","",""},
//{"Status_Flag","","",""},
//{"Time_until_absorb_charge","mdi:solar-power-variant","s","duration"},
//{"Time_until_float_charge","mdi:solar-power-variant","s","duration"},
{"Tracker_temperature","mdi:thermometer-lines","C","temperature"},
{"Transformer_temperature","mdi:thermometer-lines","C","temperature"},
{"Warning_Code","mdi:alert-outline","",""}
      };

/** later

 * @brief callback functions for debug
 * 

void debugcallback(const __FlashStringHelper *logmessage);
void debugcallbackLN(const __FlashStringHelper *logmessage);
 */