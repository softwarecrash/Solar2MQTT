#ifndef MODBUS_ANENJI_H
#define MODBUS_ANENJI_H

#include <modbus/device/modbus_device.h>

/*
    should work with devices on https://powmr.com/products/powmr-wifi-module-wifi-6-2-lip dongle

    also possible should work 
    ISolar SMG II
    EASUN SMG II
    PowMr POW-HVM5.5K-48V / POW-HVM5.5K-48V-P
*/

class Anenji : public ModbusDevice
{
public:
    Anenji() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
     static const long _baudRate = 9600;
     static const uint32_t _modbusAddr = 1;
     static const protocol_type_t _protocol = MODBUS_ANENJI;
    inline static const char *const _name = "Anenji";

    inline static const modbus_register_t registers_live[] = {
        {201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Inverter_Operation_Mode, 0, {.bitfield = {
                                                                                                             "Power On",
                                                                                                             "Standby",
                                                                                                             "Mains",
                                                                                                             "Off-Grid",
                                                                                                             "Bypass",
                                                                                                             "Charging",
                                                                                                             "Fault",
                                                                                                         }}},
                                                                                                         
         {202, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_In_Voltage},
         {203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_In_Frequenz}, 
         {204, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Input_Power}, 
        {205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_Out_Voltage},
        {206, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Output_current},
        {207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_Out_Frequenz},
        {208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Output_power},
        {209, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Inverter_Charging_Power},
        {210, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_Out_Effective_Voltage},
        {211, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Output_effective_current},
        {212, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_Out_Frequenz},
        {213, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_Rating_Active_Power},
        {214, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_Rating_Apparent_Power}, 
        {215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_avg_Voltage},     
        {216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_avg_Current},
        {217, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Battery_avg_Power},
        {219, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Input_Voltage},
        {220, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Input_Current},
        {223, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Input_Power},
        {224, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV_Charging_Power},
        {225, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Output_load_percent}, 
        {226, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Transformer_Temperature},
        {227, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_Inverter_Temperature},
        {229, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Battery_Percent},
        {232, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Charge_Average_Current},
        {233, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Inverter_avg_Charge_Current},
        {234, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Charging_avg_Power},
    };

    inline static const modbus_register_t registers_static[] = { 
     
        {300, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Output_Mode, 0, {.bitfield = {
                                                                                    "Single",
                                                                                    "Parallel",
                                                                                    "3 Phase-P1",
                                                                                    "3 Phase-P2",
                                                                                    "3 Phase-P3",
                                                                                    "Split Phase-P1",
                                                                                    "Split Phase-P2"}}},
        {301, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Output_Source_Priority, 0, {.bitfield = {
                                                                                    "Utility first (USB)",
                                                                                    "Solar first (SUB)",
                                                                                    "SBU priority",
                                                                                }}},
        {302, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Input_Voltage_Range, 0, {.bitfield = {
            "Wide", "Narrow"}}},
        {303, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Buzzer_Enabled, 0, {.bitfield = {
            "Mute", "Warning", "Fault", "Fault Only"}}},
        {305, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LCD_Backlight_Enabled, 0, {.bitfield = {
            "Timed Off", "Always On"}}},
        {306, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_LCD_Reset_To_Default_Enabled, 0, {.bitfield = {
            "Do Not Return", "Return After 1 Minute"}}},
        {307, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Power_Saving_Enabled, 0, {.bitfield = {
            "Off", "On"}}},
        {308, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Overload_Restart_Enabled, 0, {.bitfield = {
            "No Restart", "Automatic Restart"}}},
        {310, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Overload_Bypass_Enabled, 0, {.bitfield = {
            "Disable", "Enable"}}},
        {322, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Battery_Type, 0, {.bitfield = {
            "AGM", "FLD", "USER", "SMK1", "PYL", "FOX"}}},
        {320, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_output_voltage},
        {321, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_output_frequency},
        {324, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Bulk_Voltage},
        {325, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Float_Voltage},
        {327, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Battery_Under_Voltage},
        {331, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Charger_Source_Priority, 0, {.bitfield = {
            "Utility", "PV priority", "PV is at the same level as the Utility", "Only PV charging is allowed"}}},
        {332, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Current_Max_Charging_Current},
        {333, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Current_Max_AC_Charging_Current},


    }; 
};

#endif
