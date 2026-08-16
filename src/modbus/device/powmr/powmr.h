#ifndef MODBUS_POWMR_H
#define MODBUS_POWMR_H

#include <modbus/device/modbus_device.h>

class PowMr : public ModbusDevice
{
public:
    PowMr() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    const modbus_register_t *getLiveRegisters() const override;
    const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
    static constexpr long _baudRate = 2400;
    static constexpr uint32_t _modbusAddr = 5;
    static constexpr protocol_type_t _protocol = MODBUS_POWMR;
    inline static const char *const _name = "PowMr HVM";

    static constexpr uint16_t kMainBlockStart = 4501;
    static constexpr uint16_t kMainBlockCount = 45;
    static constexpr uint16_t kConfigBlockStart = 4546;
    static constexpr uint16_t kConfigBlockCount = 16;

    inline static const modbus_register_t registers_live[] = {
        {4501, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Inverter_Operation_Mode, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4502, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_AC_In_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4503, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_AC_In_Frequency, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4504, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_PV_Input_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4505, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_PV_Input_Power, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4506, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4507, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Battery_Percent, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4508, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Battery_Charge_Current, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4509, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Battery_Discharge_Current, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4510, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_AC_Out_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4511, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_AC_Out_Frequency, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4512, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_AC_Out_Watt, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4513, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_AC_Out_VA, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4514, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Output_Load_Percent, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4530, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Fault_Code, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4555, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME_SWAP, "Charger_Status", 0, {.bitfield = {"Off", "Idle", "Charging"}}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4557, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Inverter_Temperature, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
    };

    inline static const modbus_register_t registers_static[] = {
        {4535, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "PowMr_Settings_Flags", 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4536, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME_SWAP, DESCR_Charger_Source_Priority, 0, {.bitfield = {"Utility first", "Solar first", "Solar and Utility", "Solar only"}}, nullptr, kMainBlockStart, kMainBlockCount},
        {4537, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME_SWAP, DESCR_Output_Source_Priority, 0, {.bitfield = {"Utility first", "Solar first", "SBU priority"}}, nullptr, kMainBlockStart, kMainBlockCount},
        {4538, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME_SWAP, DESCR_Input_Voltage_Range, 0, {.bitfield = {"Appliances", "UPS"}}, nullptr, kMainBlockStart, kMainBlockCount},
        {4540, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME_SWAP, DESCR_AC_Out_Rating_Frequency, 0, {.bitfield = {"50", "60"}}, nullptr, kMainBlockStart, kMainBlockCount},
        {4541, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Current_Max_Charging_Current, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4542, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_AC_Out_Rating_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4543, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, DESCR_Current_Max_AC_Charging_Current, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4544, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Recharge_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4545, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Redischarge_Voltage, 0, {}, nullptr, kMainBlockStart, kMainBlockCount},
        {4546, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Bulk_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4547, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Float_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4548, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, DESCR_Battery_Under_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4549, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP_ONE_DECIMAL, "Battery_Equalization_Voltage", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4550, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "Battery_Equalization_Time", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4551, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "Battery_Equalization_Timeout", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4552, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "Battery_Equalization_Interval", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4553, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "PowMr_Status_Flags_1", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {4554, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16_SWAP, "PowMr_Status_Flags_2", 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
    };
};

#endif
