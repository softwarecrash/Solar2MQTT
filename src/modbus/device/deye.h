#ifndef MODBUS_DEYE_H
#define MODBUS_DEYE_H
#include "modbus_device.h"
#include <PI_Serial/PI_Serial.h>

class DEYE : public ModbusDevice
{
public:
    DEYE() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override
    {
        return registers_live;
    }

    virtual const modbus_register_t *getStaticRegisters() const override
    {
        return registers_static;
    }

    const char *getName() const override
    {
        return _name;
    }

    bool retrieveModel(MODBUS &modbus, char *modelBuffer, size_t bufferSize) override;

private:
    static const long _baudRate = 9600;
    static const uint32_t _modbusAddr = 1;
    static const protocol_type_t _protocol = MODBUS_DEYE;
    static const char *const _name;

    static const modbus_register_t registers_live[];
    static const modbus_register_t registers_static[];
    static const modbus_register_t registers_device_serial[];
};

const char *const DEYE::_name = "DEYE";

const modbus_register_t DEYE::registers_live[] = { 
    {175, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_LIVE_OUTPUT_POWER},
    {178, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_LIVE_AC_OUTPUT_POWER},

    {182, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_BATTERY_TEMPERATURE},

    {183, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_LIVE_BATTERY_VOLTAGE},
    {184, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_BATTERY_PERCENT},
};

const modbus_register_t DEYE::registers_static[] = {

    {10110, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "Battery_type", {.bitfield = {
                                                                                     "No choose",
                                                                                     "User defined",
                                                                                     "Lithium",
                                                                                     "Sealed Lead",
                                                                                     "AGM",
                                                                                     "GEL",
                                                                                     "Flooded",
                                                                                 }}},
};

const modbus_register_t DEYE::registers_device_serial[] = {
    {3, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN1"},
    {4, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN2"},
    {5, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN3"},
    {6, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN4"},
    {7, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN5"}};

bool DEYE::retrieveModel(MODBUS &modbus, char *modelBuffer, size_t bufferSize)
{
    modelBuffer[0] = '\0'; // Clear the buffer
    DynamicJsonDocument doc(100);
    JsonObject jsonObj = doc.to<JsonObject>(); // Create and get JsonObject
    modbus_register_info_t model_info = {
        .variant = &jsonObj,
        .registers = registers_device_serial,
        .array_size = sizeof(registers_device_serial) / sizeof(modbus_register_t),
        .curr_register = 0};

    for (size_t i = 0; i < model_info.array_size; i++)
    {
        modbus.parseModbusToJson(model_info, false);
        if (modbus.isAllRegistersRead(model_info))
        {
            const char *sn1 = doc["SN1"];
            const char *sn2 = doc["SN2"];
            const char *sn3 = doc["SN3"];
            const char *sn4 = doc["SN4"];
            const char *sn5 = doc["SN5"];
            snprintf(modelBuffer, bufferSize, "%s%s%s%s%s", sn1, sn2, sn3, sn4, sn5);
            return true;
        }
        delay(50);
    }
    return false;
}

#endif