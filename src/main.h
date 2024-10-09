#include <WebSerialLite.h>
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#define JSON_BUFFER 2048


#ifdef isUART_HARDWARE
#define INVERTER_TX 1
#define INVERTER_RX 3
#else
#define INVERTER_TX 13
#define INVERTER_RX 12
#define TEMPSENS_PIN 4 // DS18B20 Pin; D2 on Wemos D1 Mini
#endif

#define LED_PIN 02 // D4 with the LED on Wemos D1 Mini
#define LED_COM 5
#define LED_SRV 0
#define LED_NET 4

#define DBG_BAUD 115200
#define DBG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#define SOFTWARE_VERSION SWVERSION
#define DBG Serial
#define DBG_BEGIN(...) DBG.begin(__VA_ARGS__)
#define DBG_PRINTLN(...) DBG.println(__VA_ARGS__)


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

static const char *const haStaticDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {"AC_in_rating_current", "current-ac", "A", "current"},
    {"AC_in_rating_voltage", "flash-triangle-outline", "V", "voltage"},
    {"AC_out_rating_active_power", "sine-wave", "W", "power"},
    {"AC_out_rating_apparent_power", "sine-wave", "W", "power"},
    {"AC_out_rating_current", "current-ac", "A", "current"},
    {"AC_out_rating_frequency", "sine-wave", "Hz", "frequency"},
    {"AC_out_rating_voltage", "flash-triangle-outline", "V", "voltage"},
    {"Battery_bulk_voltage", "car-battery", "V", "voltage"},
    {"Battery_float_voltage", "car-battery", "V", "voltage"},
    {"Battery_rating_voltage", "car-battery", "V", "voltage"},
    {"Battery_re-charge_voltage", "battery-charging-high", "V", "voltage"},
    {"Battery_re-discharge_voltage", "battery-charging-outline", "V", "voltage"},
    {"Battery_type", "car-battery", "", ""},
    {"Battery_under_voltage", "battery-remove-outline", "V", "voltage"},
    {"Charger_source_priority", "ev-station", "", ""},
    {"Current_max_AC_charging_current", "current-ac", "A", "current"},
    {"Current_max_charging_current", "battery-charging", "A", "current"},
    {"Device_Model", "battery-charging", "", ""},
    {"Input_voltage_range", "flash-triangle-outline", "", ""},
    {"Machine_type", "state-machine", "", ""},
    {"Max_charging_time_at_CV_stage", "clock-time-eight-outli", "s", "duration"},
    {"Max_discharging_current", "battery-outline", "A", "current"},
    {"MPPT_string", "string-lights", "", ""},
    {"Operation_Logic", "access-point", "", ""},
    {"Output_mode", "export", "", ""},
    {"Output_source_priority", "export", "", ""},
    //{"Parallel_max_num","","",""},
    {"Protocol_ID", "protocol", "", ""},
    //{"PV_OK_condition_for_parallel","solar-panel","",""},
    {"PV_power_balance", "solar-panel", "", ""},
    {"Solar_power_priority", "priority-high", "", ""},
    {"Topology", "earth", "", ""}};
static const char *const haLiveDescriptor[][4]{
    // state_topic, icon, unit_ofmeasurement, class
    {"AC_in_Frequenz", "import", "Hz", "frequency"},
    {"AC_in_generation_day", "import", "Wh", "energy"},
    {"AC_in_generation_month", "import", "Wh", "energy"},
    {"AC_in_generation_sum", "import", "Wh", "energy"},
    {"AC_in_generation_year", "import", "Wh", "energy"},
    {"AC_in_Voltage", "import", "V", "voltage"},
    {"AC_out_Frequenz", "export", "Hz", "frequency"},
    {"AC_out_percent", "export", "%", "power_factor"},
    {"AC_out_VA", "export", "VA", "apparent_power"},
    {"AC_out_Voltage", "export", "V", "voltage"},
    {"AC_out_Watt", "export", "W", "power"},
    {"AC_output_current", "export", "A", "current"},
    {"AC_output_frequency", "export", "Hz", "frequency"},
    {"AC_output_power", "export", "W", "power"},
    {"AC_output_voltage", "export", "V", "voltage"},
    //{"ACDC_Power_Direction","sign-direction","",""},
    {"Battery_capacity", "battery-high", "%", "battery"},
    //{"Battery_Charge_Current","battery-charging-high","A","current"},
    //{"Battery_Discharge_Current","battery-charging-outli","A","current"},
    {"Battery_Load", "battery-charging-high", "A", "current"},
    {"Battery_Percent", "battery-charging-high", "%", "battery"},
    {"Battery_Power_Direction", "battery-charging-high", "", ""},
    //{"Battery_SCC_Volt","battery-high","V","voltage"},
    //{"Battery_SCC2_Volt","battery-high","V","voltage"},
    {"Battery_temperature", "thermometer-lines", "°C", "temperature"},
    {"Battery_Voltage", "battery-high", "V", "voltage"},
    //{"Battery_voltage_offset_fans_on","fan","",""},
    //{"Configuration_State","state-machine","",""},
    //{"Country","earth","",""},
    //{"Device_Status","state-machine","",""},
    //{"EEPROM_Version","chip","",""},
    {"Fan_speed","fan","%",""},
    {"Fault_code","alert-outline","",""},
    {"Grid_frequency", "import", "Hz", "frequency"},
    {"Grid_voltage", "import", "V", "voltage"},
    {"Inverter_Bus_Temperature", "thermometer-lines", "°C", "temperature"},
    {"Inverter_Bus_Voltage", "flash-triangle-outline", "V", "voltage"},
    //{"Inverter_charge_state","car-turbocharger","",""},
    {"Inverter_Operation_Mode", "car-turbocharger", "", ""},
    {"Inverter_temperature", "thermometer-lines", "°C", "temperature"},
    //{"Line_Power_Direction","transmission-tower","",""},
    //{"Load_Connection","connection","",""},
    {"Local_Parallel_ID", "card-account-details-outline", "", ""},
    //{"Max_temperature","thermometer-plus","C","temperature"},
    //{"MPPT1_Charger_Status","car-turbocharger","",""},
    {"MPPT1_Charger_Temperature", "thermometer-lines", "°C", "temperature"},
    //{"MPPT2_CHarger_Status","car-turbocharger","",""},
    {"MPPT2_Charger_Temperature", "thermometer-lines", "°C", "temperature"},
    {"Negative_battery_voltage", "battery-minus-outline", "V", "voltage"},
    {"Output_current", "export", "A", "current"},
    {"Output_load_percent", "export", "%", "battery"},
    {"Output_power", "export", "W", "power"},
    //{"PBUS_voltage","","V","voltage"},
    {"Positive_battery_voltage", "car-battery", "V", "voltage"},
    {"PV_Charging_Power", "solar-power-variant", "W", "power"},
    {"PV_generation_day", "solar-power-variant", "Wh", "energy"},
    {"PV_generation_month", "solar-power-variant", "Wh", "energy"},
    {"PV_generation_sum", "solar-power-variant", "Wh", "energy"},
    {"PV_generation_year", "solar-power-variant", "Wh", "energy"},
    {"PV_Input_Current", "solar-power-variant", "A", "current"},
    {"PV_Input_Power", "solar-power-variant", "W", "power"},
    {"PV_Input_Voltage", "solar-power-variant", "V", "voltage"},
    {"PV1_input_power", "solar-power-variant", "W", "power"},
    {"PV1_input_voltage", "solar-power-variant", "V", "voltage"},
    {"PV2_Charging_Power", "solar-power-variant", "W", "power"},
    {"PV2_Input_Current", "solar-power-variant", "A", "current"},
    {"PV2_input_power", "solar-power-variant", "W", "power"},
    {"PV2_input_voltage", "solar-power-variant", "V", "voltage"},
    {"PV3_input_power", "solar-power-variant", "W", "power"},
    {"PV3_input_voltage", "solar-power-variant", "V", "voltage"},
    //{"SBUS_voltage","flash-triangle-outline","V","voltage"},
    {"Solar_feed_to_grid_power", "solar-power-variant", "W", "power"},
    {"Solar_feed_to_Grid_status", "solar-power-variant", "", ""},
    //{"Status_Flag","flag","",""},
    //{"Time_until_absorb_charge","solar-power-variant","s","duration"},
    //{"Time_until_float_charge","solar-power-variant","s","duration"},
    {"Tracker_temperature", "thermometer-lines", "°C", "temperature"},
    {"Transformer_temperature", "thermometer-lines", "°C", "temperature"},
    {"Warning_Code", "alert-outline", "", ""}};