#ifndef MODBUS_SMG_II_11KW_H
#define MODBUS_SMG_II_11KW_H

#include <modbus/device/modbus_device.h>

#define DESCR_SMG_PROTOCOL_NUMBER "SMG_Protocol_Number"

class SMGII11KW : public ModbusDevice
{
public:
    SMGII11KW() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
    static constexpr long _baudRate = 9600;
    static constexpr uint32_t _modbusAddr = 1;
    static constexpr protocol_type_t _protocol = MODBUS_SMG_II_11KW;
    inline static const char *const _name = "SMG-II 11KW";

    static constexpr uint16_t kFaultBlockStart = 100;
    static constexpr uint16_t kFaultBlockCount = 2;
    static constexpr uint16_t kWarningBlockStart = 108;
    static constexpr uint16_t kWarningBlockCount = 2;
    static constexpr uint16_t kProtocolRegister = 184;
    static constexpr uint16_t kSerialBlockStart = 186;
    static constexpr uint16_t kSerialBlockCount = 12;
    static constexpr uint16_t kModeBlockStart = 201;
    static constexpr uint16_t kModeBlockCount = 1;
    static constexpr uint16_t kOutputBlockStart = 254;
    static constexpr uint16_t kOutputBlockCount = 3;
    static constexpr uint16_t kBatteryBlockStart = 277;
    static constexpr uint16_t kBatteryBlockCount = 5;
    static constexpr uint16_t kPvBlockStart = 302;
    static constexpr uint16_t kPvBlockCount = 2;
    static constexpr uint16_t kPv1BlockStart = 351;
    static constexpr uint16_t kPv1BlockCount = 3;
    static constexpr uint16_t kPv2BlockStart = 389;
    static constexpr uint16_t kPv2BlockCount = 3;
    static constexpr uint16_t kOutputEffectiveBlockStart = 605;
    static constexpr uint16_t kOutputEffectiveBlockCount = 3;

    inline static const modbus_register_t registers_live[] = {
        {100, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U32_HIGH_FIRST, DESCR_Fault_Code, 0, {}, nullptr, kFaultBlockStart, kFaultBlockCount},
        {108, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U32_HIGH_FIRST, DESCR_Warning_Code, 0, {}, nullptr, kWarningBlockStart, kWarningBlockCount},
        {201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Inverter_Operation_Mode, 0, {.bitfield = {
                                                                                                             "Power On",
                                                                                                             "Standby",
                                                                                                             "Mains",
                                                                                                             "Off-Grid",
                                                                                                             "Bypass",
                                                                                                             "Charging",
                                                                                                             "Fault",
                                                                                                         }},
         nullptr, kModeBlockStart, kModeBlockCount},
        {203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_TWO_DECIMAL, DESCR_AC_In_Frequency},
        {231, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Inverter_Temperature},
        {254, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_Watt, 0, {}, nullptr, kOutputBlockStart, kOutputBlockCount},
        {255, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_VA, 0, {}, nullptr, kOutputBlockStart, kOutputBlockCount},
        {256, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Output_Load_Percent, 0, {}, nullptr, kOutputBlockStart, kOutputBlockCount},
        {277, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_Voltage, 0, {}, nullptr, kBatteryBlockStart, kBatteryBlockCount},
        {278, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_Load, 0, {}, nullptr, kBatteryBlockStart, kBatteryBlockCount},
        {280, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Battery_Percent, 0, {}, nullptr, kBatteryBlockStart, kBatteryBlockCount},
        {281, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_DCDC_Temperature, 0, {}, nullptr, kBatteryBlockStart, kBatteryBlockCount},
        {302, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Input_Power, 0, {}, nullptr, kPvBlockStart, kPvBlockCount},
        {303, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Charging_Power, 0, {}, nullptr, kPvBlockStart, kPvBlockCount},
        {338, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_In_Voltage},
        {351, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Input_Voltage, 0, {}, nullptr, kPv1BlockStart, kPv1BlockCount},
        {352, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Input_Current, 0, {}, nullptr, kPv1BlockStart, kPv1BlockCount},
        {353, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV1_Input_Power, 0, {}, nullptr, kPv1BlockStart, kPv1BlockCount},
        {389, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV2_Input_Voltage, 0, {}, nullptr, kPv2BlockStart, kPv2BlockCount},
        {390, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV2_Input_Current, 0, {}, nullptr, kPv2BlockStart, kPv2BlockCount},
        {391, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV2_Input_Power, 0, {}, nullptr, kPv2BlockStart, kPv2BlockCount},
        {605, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Output_Current, 0, {}, nullptr, kOutputEffectiveBlockStart, kOutputEffectiveBlockCount},
        {606, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_Out_Voltage, 0, {}, nullptr, kOutputEffectiveBlockStart, kOutputEffectiveBlockCount},
        {607, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_TWO_DECIMAL, DESCR_AC_Out_Frequency, 0, {}, nullptr, kOutputEffectiveBlockStart, kOutputEffectiveBlockCount},
    };

    inline static const modbus_register_t registers_static[] = {
        {184, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_SMG_PROTOCOL_NUMBER},
    };
};

#endif
