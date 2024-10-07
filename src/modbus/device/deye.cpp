#include "deye.h"

const long Deye::_baudRate = 9600;
const uint32_t Deye::_modbusAddr = 1;
const protocol_type_t Deye::_protocol = MODBUS_DEYE;

const char *const Deye::_name = "DEYE";

const modbus_register_t *Deye::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *Deye::getStaticRegisters() const
{
    return registers_static;
}

const char *Deye::getName() const
{
    return _name;
}

const modbus_register_t Deye::registers_live[] = {
    {59, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_LIVE_INVERTER_OPERATION_MODE, 0, {.bitfield = {
                                                                                                         "Standby",
                                                                                                         "Self Test",
                                                                                                         "Normal",
                                                                                                         "Alerts",
                                                                                                         "Fault",
                                                                                                     }}},

    {109, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_VOLTAGE, 0},
    {110, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_CURRENT, 0},
    {186, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_PV_INPUT_POWER, 0},

    {175, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_LIVE_OUTPUT_POWER, 0},
    {178, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_LIVE_AC_OUTPUT_POWER, 0},

    {90, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_INVERTER_TEMPERATURE, -100},
    {182, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_BATTERY_TEMPERATURE, -100},

    {183, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_LIVE_BATTERY_VOLTAGE, 0},
    {184, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_BATTERY_PERCENT, 0},
};

const modbus_register_t Deye::registers_static[] = {
    {16, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U32_ONE_DECIMAL, DESCR_STAT_AC_OUT_RATING_ACTIVE_POWER, 0},
};

const modbus_register_t Deye::registers_device_serial[] = {
    {3, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN1"},
    {4, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN2"},
    {5, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN3"},
    {6, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN4"},
    {7, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN5"}};

bool Deye::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
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
        mCom.parseModbusToJson(model_info, false);
        if (mCom.isAllRegistersRead(model_info))
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

// Define the size calculation after the arrays are defined
size_t Deye::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t Deye::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}