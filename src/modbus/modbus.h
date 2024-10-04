#ifndef MODBUS_H
#define MODBUS_H

#include "SoftwareSerial.h"
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include "modbus_registers.h"
#include "device/modbus_device.h"

extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

#define RS485_DIR_PIN 14 // D5
#define RS485_ESP01_DIR_PIN 0

#define MAX485_DONGLE_DE_PIN       5         // D1, DE pin on the TTL to RS485 converter
#define MAX485_DONGLE_RE_NEG_PIN   4  

#define RS485_BAUDRATE 19200

#define INVERTER_MODBUS_ADDR 4

#define MODBUS_RETRIES 2

typedef enum
{
    READ_FAIL = 0,
    READ_OK = 1,
} response_type_t;

typedef struct
{
    JsonObject *variant;
    const modbus_register_t *registers;
    uint8_t array_size;
    uint8_t curr_register;
} modbus_register_info_t;

class MODBUS
{
public:
    const uint8_t MAX_CONNECTION_ATTEMPTS = 10;
    bool requestStaticData = true;
    bool connection = false;
    modbus_register_info_t live_info;
    modbus_register_info_t static_info;
    MODBUS(SoftwareSerial *port);

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * @brief Updating the Data from the inverter
     */
    void loop();

    /**
     * @brief Send custom command to the device
     */
    bool sendCommand(String command);

    /**
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback;
    bool readModbusRegisterToJson(const modbus_register_t *reg, JsonObject *variant);
    response_type_t parseModbusToJson(modbus_register_info_t &register_info, bool skip_reg_on_error = true);
    bool isAllRegistersRead(modbus_register_info_t &register_info);
    protocol_type_t autoDetect();
    /**
     * @brief Sends a complete packet with the specified command
     * @details sends the command over the specified serial connection
     */
    String requestData(String command);

private:
    unsigned long previousTime = 0;
    unsigned long cmdDelayTime = 100;

    byte requestCounter = 0;

    long long int connectionCounter = 0;

    byte qexCounter = 0;

    static void preTransmission();
    static void postTransmission();
    void prepareRegisters();
    String toBinary(uint16_t input);
    bool decodeDiematicDecimal(uint16_t int_input, int8_t decimals, float *value_ptr);
    bool getModbusResultMsg(uint8_t result);
    bool getModbusValue(uint16_t register_id, modbus_entity_t modbus_entity, uint16_t *value_ptr, uint16_t readBytes = 1);
    /**
     * @brief get the crc from a string
     */
    uint16_t getCRC(String data);

    /**
     * @brief get the crc from a string
     */
    byte getCHK(String data);

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    SoftwareSerial *my_serialIntf;
    ModbusDevice *device = nullptr;
    ModbusMaster mb;
};

#endif
