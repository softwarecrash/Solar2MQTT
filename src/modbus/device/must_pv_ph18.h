#ifndef MODBUS_MUST_PV_PH18_H
#define MODBUS_MUST_PV_PH18_H

#include "modbus_device.h"

#define DEVICE_MODEL_HIGH "must_device_model_h"
#define DEVICE_MODEL_LOW "must_device_model_l"

#define MUST_DEVICE_CHARGER_HIGH "ch_h"
#define MUST_DEVICE_CHARGER_LOW "ch_l"

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
    static void generationSum(JsonObject *variant, uint16_t *registerValue, const modbus_register_t *reg, MODBUS_COM &mCom);

private:
    static const long _baudRate = 19200;
    static const uint32_t _modbusAddr = 4;
    static const protocol_type_t _protocol = MODBUS_MUST;
    inline static const char *const _name = "MUST PV/PH 18";

    inline static const modbus_register_t registers_live[] = {
        {0, MODBUS_TYPE_HOLDING, REGISTER_TYPE_VIRTUAL_CALLBACK, DESCR_LIVE_PV_GENERATION_SUM, 0, {}, MustPV_PH18::generationSum},
        {15201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_LIVE_INVERTER_OPERATION_MODE, 0, {.bitfield = {
                                                                                                                "Power On",
                                                                                                                "Self Test",
                                                                                                                "OffGrid",
                                                                                                                "GridTie",
                                                                                                                "ByPass",
                                                                                                                "Stop",
                                                                                                                "GridCharging",
                                                                                                            }}},
        {15205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_VOLTAGE},
        {15207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_PV_INPUT_CURRENT},
        {15208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_PV_CHARGING_POWER},
        {15209, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_MPPT1_CHARGER_TEMPERATURE},
        {15212, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "PV_Relay"},
        {15217, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, MUST_DEVICE_CHARGER_HIGH},
        {15218, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, MUST_DEVICE_CHARGER_LOW},
        {15219, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_PV_GENERATION_DAY},

        {25201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "PV_Charger_Workstate", 0, {.bitfield = {"Initializtion", "Self Test", "Work", "Stop"}}},
        {25202, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "PV_Charger_MPPT_State", 0, {.bitfield = {
                                                                                                     "Stop",
                                                                                                     "MPPT",
                                                                                                     "Current Limit",
                                                                                                 }}},
        {25203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "PV_Charger_Charge_State", 0, {.bitfield = {
                                                                                                       "Stop",
                                                                                                       "Absorb",
                                                                                                       "Float",
                                                                                                       "EQ",
                                                                                                   }}},
        {25205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_BATTERY_VOLTAGE},
        {25206, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_AC_OUT_VOLTAGE},
        {25207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_AC_IN_VOLTAGE},
        {25208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_INVERTER_BUS_VOLTAGE},
        {25210, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_OUTPUT_CURRENT},
        {25211, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_LIVE_AC_OUTPUT_CURRENT},
        {25212, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Inverter_Load_Current"},
        {25213, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_OUTPUT_POWER},
        {25214, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "AC_in_Power"},
        {25215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_AC_OUT_WATT}, // W
        {25216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_AC_OUT_PERCENT}, 
        {25225, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_LIVE_AC_OUT_FREQUENZ},
        {25226, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_LIVE_AC_IN_FREQUENZ},
        {25233, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_INVERTER_BUS_TEMPERATURE},
        {25234, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LIVE_TRANSFORMER_TEMPERATURE},
        {25274, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_LIVE_BATTERY_LOAD},
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
        {10103, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Battery_float_voltage"},
    };

    inline static const modbus_register_t registers_device_model[] = {
        {20000, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, DEVICE_MODEL_HIGH},
        {20001, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DEVICE_MODEL_LOW, 0}};
};

#endif
