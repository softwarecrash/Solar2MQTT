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
static const char *const haDescriptor[][4]{
      // state_topic, icon, unit_ofmeasurement, class
      {"Voltage", "mdi:car-battery", "V", "voltage"},
      {"Voltage_2", "mdi:car-battery", "V", "voltage"},
      {"Voltage_3", "mdi:car-battery", "V", "voltage"},
      {"Starter_voltage", "mdi:flash-triangle-outline", "V", "voltage"},
      {"Mid_voltage", "mdi:battery-outline", "V", "voltage"},
      {"Mid_deviation", "", "%", ""},
      {"Panel_voltage", "mdi:solar-panel", "V", "voltage"},
      {"Battery_current", "mdi:current-dc", "A", "current"},
      {"Battery_current_2", "mdi:current-dc", "A", "current"},
      {"Battery_current_3", "mdi:current-dc", "A", "current"},
      {"Load_current", "mdi:battery-charging", "A", "current"},
      {"Load_output_state", "mdi:export", "", ""},
      {"Battery_temperature", "mdi:thermometer", "C", "thermometer"},
      {"Instantaneous_power", "mdi:solar-power", "W", "power"},
      {"Consumed_Amp_Hours", "mdi:home-battery-outline", "Ah", ""},
      {"SOC", "mdi:battery-charging", "%", "battery"},
      {"Time_to_go", "mdi:camera-timer", "min", "duration"},
      {"Alarm", "mdi:alarm-light-outline", "", ""},
      {"Relay", "mdi:electric-switch", "", ""},
      {"Alarm_code", "mdi:alarm-panel-outline", "", ""},
      {"Off_reason", "mdi:alarm-panel-outline", "", ""},
      {"Deepest_discharge", "mdi:battery-outline", "Ah", ""},
      {"Last_discharge", "mdi:battery-outline", "Ah", ""},
      {"Average_discharge", "mdi:battery-outline", "Ah", ""},
      {"Charge_cycles", "mdi:counter", "", ""},
      {"Full_discharges", "mdi:counter", "", ""},
      {"Cumulative_Ah_drawn", "mdi:battery-outline", "Ah", ""},
      {"Minimum_voltage", "mdi:battery-charging-outline", "V", "voltage"},
      {"Maximum_voltage", "mdi:battery-charging-high", "V", "voltage"},
      {"Last_full_charge", "mdi:clock-time-eight-outline", "h", "duration"},
      {"Num_automatic_sync", "mdi:counter", "", ""},
      {"Num_low_volt_alarms", "mdi:counter", "", ""},
      {"Num_high_volt_alarms", "mdi:counter", "", ""},
      {"Num_low_aux_vol_alarms", "mdi:counter", "", ""},
      {"Num_high_aux_vol_alarms", "mdi:counter", "", ""},
      {"Min_aux_volt", "mdi:battery-low", "V", "voltage"},
      {"Max_aux_volt", "mdi:battery-high", "V", "voltage"},
      {"Amount_discharged_energy", "mdi:battery-arrow-down-outline", "kWh", "energy"},
      {"Amount_charged_energy", "mdi:battery-arrow-up-outline", "kWh", "energy"},
      {"total_kWh", "mdi:solar-power", "kWh", "energy"},
      {"today_kWh", "mdi:solar-power", "kWh", "energy"},
      {"Max_pow_today", "mdi:solar-power-variant-outline", "W", "power"},
      {"Yesterday_kWh", "mdi:solar-power", "kWh", "energy"},
      {"Max_pow_yesterday", "mdi:solar-power-variant-outline", "W", "power"},
      {"Current_error", "mdi:alert-circle-outline", "", ""},
      {"Operation_state", "", "", ""},
      {"Model_description", "", "", ""},
      {"Firmware_version_16", "", "", ""},
      {"Firmware_version_24", "", "", ""},
      {"Device_model", "", "", ""},
      {"Serial_number", "", "", ""},
      {"Day", "", "", ""},
      {"Device_mode", "", "", ""},
      {"AC_out_volt", "mdi:current-ac", "V", "voltage"},
      {"AC_out_current", "mdi:current-ac", "A", "current"},
      {"AC_out_apparent_pow", "mdi:current-ac", "VA", "apparent_power"},
      {"Warning_reason", "", "", ""},
      {"Tracker_operation_mode", "", "", ""},
      {"DC_monitor_mode", "", "", ""},
      {"DC_input_voltage", "mdi:current-dc", "V", "voltage"},
      {"DC_input_current", "mdi:current-dc", "A", "current"},
      {"DC_input_power", "mdi:current-dc", "W", "power"}
      };

/** later

 * @brief callback functions for debug
 * 

void debugcallback(const __FlashStringHelper *logmessage);
void debugcallbackLN(const __FlashStringHelper *logmessage);
 */