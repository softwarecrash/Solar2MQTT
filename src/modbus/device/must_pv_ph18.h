#ifndef MODBUS_MUST_PV_PH18_H
#define MODBUS_MUST_PV_PH18_H

#include "modbus_device.h"

#define DEVICE_MODEL_HIGH "Device_Model_Hight"
#define DEVICE_MODEL_LOW "Device_Model_Low"

class MustPV_PH18 : public ModbusDevice
{
public: 
    MustPV_PH18() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
     static const long _baudRate = 19200;
     static const uint32_t _modbusAddr = 4;
     static const protocol_type_t _protocol = MODBUS_MUST;
    inline static const char *const _name = "MUST PV/PH 18";

    inline static const modbus_register_t registers_live[] = {
        {25201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_LIVE_INVERTER_OPERATION_MODE, 0, {.bitfield = {
                                                                                                                "Power On",
                                                                                                                "Self Test",
                                                                                                                "OffGrid",
                                                                                                                "GridTie",
                                                                                                                "ByPass",
                                                                                                                "Stop",
                                                                                                                "GridCharging",
                                                                                                            }}},

        {25205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_BATTERY_VOLTAGE, 0},
        {25206, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "AC_out_Voltage", 0},
        {25207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "AC_in_Voltage", 0},
        {25208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Inverter_Bus_Voltage", 0},
        {25225, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "AC_out_Frequenz", 0},
        {25226, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "AC_in_Frequenz", 0},

        {25216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Output_load_percent", 0},
        {15205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_VOLTAGE, 0},
        {15208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "PV_Charging_Power", 0},
        {15207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_CURRENT, 0},
        {25233, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Inverter_Bus_Temperature", 0},
        {25234, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Transformer_temperature", 0},
        {15209, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "MPPT1_Charger_Temperature", 0},

        {25215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "AC_out_Watt", 0},    // W
        {25216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "AC_out_percent", 0}, //%

        {25274, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Battery_Load", 0},
    };

    inline static const modbus_register_t registers_static[] = {
        {10110, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "Battery_type", 0, {.bitfield = {
                                                                                            "No choose",
                                                                                            "User defined",
                                                                                            "Lithium",
                                                                                            "Sealed Lead",
                                                                                            "AGM",
                                                                                            "GEL",
                                                                                            "Flooded",
                                                                                        }}},
        {10103, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Battery_float_voltage", 0},
    };

    inline static const modbus_register_t registers_device_model[] = {
        {20000, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, DEVICE_MODEL_HIGH, 0},
        {20001, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DEVICE_MODEL_LOW, 0}};
};

#endif
