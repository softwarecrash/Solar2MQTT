#ifdef isDEBUG
#define DEBUG Serial // Uncomment the below #define to enable debugging print statements.
//#include <WebSerialLite.h>
#endif
#define ARDUINOJSON_USE_DOUBLE 1
#define ARDUINOJSON_USE_LONG_LONG 1
#include <WebSerialLite.h>

#define INVERTER_TX 13
#define INVERTER_RX 12

#define LED_PIN 02 //D4 with the LED on Wemos D1 Mini

#define JSON_BUFFER 4096
#define MQTT_BUFFER 512

#define ESP01
#define FlashSize ESP.getFreeSketchSpace()
#ifdef ARDUINO_ESP8266_ESP01
#undef EPEVER_DE_RE
#define EPEVER_DE_RE 0  // ESP01 
#ifdef ESP01
#undef ESP01
#define ESP01 "display: none;"
#endif 
#endif

// DON'T edit version here, place version number in platformio.ini (custom_prog_version) !!!
#define SOFTWARE_VERSION SWVERSION
#ifdef isDEBUG
#undef SOFTWARE_VERSION
#define SOFTWARE_VERSION SWVERSION " " HWBOARD " " __DATE__ " " __TIME__
#endif

#define DEBUG_BAUD 115200
#ifdef DEBUG
#define DEBUG_BEGIN(...) DEBUG.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) DEBUG.print(__VA_ARGS__)
#define DEBUG_WEB(...) WebSerial.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) DEBUG.printf(__VA_ARGS__)
#define DEBUG_WEBF(...) WebSerial.printf(__VA_ARGS__)
#define DEBUG_WRITE(...) DEBUG.write(__VA_ARGS__)
#define DEBUG_PRINTLN(...) DEBUG.println(__VA_ARGS__)
#define DEBUG_WEBLN(...) WebSerial.println(__VA_ARGS__)
#else
#undef DEBUG_BEGIN
#undef DEBUG_PRINT
#undef DEBUG_WEB
#undef DEBUG_PRINTF
#undef DEBUG_WEBF
#undef DEBUG_WRITE
#undef DEBUG_PRINTLN
#undef DEBUG_WEBLN
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_WEB(...)
#define DEBUG_PRINTF(...)
#define DEBUG_WEBF(...)
#define DEBUG_WRITE(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_WEBLN(...)
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

/** later

 * @brief callback functions for debug
 * 

void debugcallback(const __FlashStringHelper *logmessage);
void debugcallbackLN(const __FlashStringHelper *logmessage);
 */