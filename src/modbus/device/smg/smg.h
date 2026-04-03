#ifndef MODBUS_SMG_H
#define MODBUS_SMG_H

#include <modbus/device/modbus_device.h>

class SMG : public ModbusDevice
{
public:
    SMG() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
    static constexpr long _baudRate = 9600;
    static constexpr uint32_t _modbusAddr = 1;
    static constexpr protocol_type_t _protocol = MODBUS_SMG;
    inline static const char *const _name = "SMG";

    static constexpr uint16_t kStatusBlockStart = 100;
    static constexpr uint16_t kStatusBlockCount = 10;
    static constexpr uint16_t kLiveBlockStart = 201;
    static constexpr uint16_t kLiveBlockCount = 34;
    static constexpr uint16_t kConfigBlockStart = 300;
    static constexpr uint16_t kConfigBlockCount = 38;
    static constexpr uint16_t kSerialBlockStart = 186;
    static constexpr uint16_t kSerialBlockCount = 12;

    inline static const modbus_register_t registers_live[] = {
        {100, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U32_HIGH_FIRST, DESCR_Fault_Code, 0, {}, nullptr, kStatusBlockStart, kStatusBlockCount},
        {108, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U32_HIGH_FIRST, DESCR_Warning_Code, 0, {}, nullptr, kStatusBlockStart, kStatusBlockCount},
        {201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Inverter_Operation_Mode, 0, {.bitfield = {
                                                                                                             "Power On",
                                                                                                             "Standby",
                                                                                                             "Mains",
                                                                                                             "Off-Grid",
                                                                                                             "Bypass",
                                                                                                             "Charging",
                                                                                                             "Fault",
                                                                                                         }},
         nullptr, kLiveBlockStart, kLiveBlockCount},
        {202, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_In_Voltage, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_In_Frequency, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {210, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_Out_Voltage, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {211, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Output_Current, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {212, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_Out_Frequency, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {213, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_AC_Out_Watt, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {214, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_AC_Out_VA, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Voltage, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {219, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_PV_Input_Voltage, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {220, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_PV_Input_Current, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {223, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Input_Power, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {224, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Charging_Power, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {225, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Output_Load_Percent, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {226, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_DCDC_Temperature, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {227, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Inverter_Temperature, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {229, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Battery_Percent, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {232, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Load, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
        {233, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Charge_Average_Current, 0, {}, nullptr, kLiveBlockStart, kLiveBlockCount},
    };

    inline static const modbus_register_t registers_static[] = {
        {300, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Output_Mode, 0, {.bitfield = {
                                                                                                   "Single",
                                                                                                   "Parallel",
                                                                                                   "3 Phase-P1",
                                                                                                   "3 Phase-P2",
                                                                                                   "3 Phase-P3",
                                                                                               }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {301, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Output_Source_Priority, 0, {.bitfield = {
                                                                                                              "Utility-PV-Battery",
                                                                                                              "PV-Utility-Battery",
                                                                                                              "PV-Battery-Utility",
                                                                                                          }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {302, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Input_Voltage_Range, 0, {.bitfield = {
                                                                                                           "Wide range",
                                                                                                           "Narrow range",
                                                                                                       }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {305, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_LCD_Backlight_Enabled, 0, {.bitfield = {
                                                                                                             "Timed off",
                                                                                                             "Always on",
                                                                                                         }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {307, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Power_Saving_Enabled, 0, {.bitfield = {
                                                                                                            "Off",
                                                                                                            "On",
                                                                                                        }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {308, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Overload_Restart_Enabled, 0, {.bitfield = {
                                                                                                                 "Off",
                                                                                                                 "On",
                                                                                                             }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {309, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Over_Temperature_Restart_Enabled, 0, {.bitfield = {
                                                                                                                         "Off",
                                                                                                                         "On",
                                                                                                                     }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {310, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Overload_Bypass_Enabled, 0, {.bitfield = {
                                                                                                                "Off",
                                                                                                                "On",
                                                                                                            }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {320, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_Out_Rating_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {321, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_Out_Rating_Frequency, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {324, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Bulk_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {325, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Float_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {326, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Redischarge_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {327, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Under_Voltage, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {331, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Charger_Source_Priority, 0, {.bitfield = {
                                                                                                               "Utility priority",
                                                                                                               "PV priority",
                                                                                                               "PV and Utility",
                                                                                                               "PV only",
                                                                                                           }},
         nullptr, kConfigBlockStart, kConfigBlockCount},
        {332, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Current_Max_Charging_Current, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
        {333, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Current_Max_AC_Charging_Current, 0, {}, nullptr, kConfigBlockStart, kConfigBlockCount},
    };
};

#endif
