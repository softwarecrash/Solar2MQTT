#ifndef MODBUS_ANENJI_SRNE_H
#define MODBUS_ANENJI_SRNE_H

#include <modbus/device/modbus_device.h>

#define DESCR_SRNE_PRODUCT_INFO "SRNE_Product_Info"

class AnenjiSrne : public ModbusDevice
{
public:
    AnenjiSrne() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;
    static void productInfo(JsonObject *variant, uint16_t *registerValue, const modbus_register_t *reg, MODBUS_COM &mCom);

private:
    static constexpr long _baudRate = 9600;
    static constexpr uint32_t _modbusAddr = 1;
    static constexpr protocol_type_t _protocol = MODBUS_ANENJI_SRNE;
    inline static const char *const _name = "Anenji SRNE";

    static constexpr uint16_t kProductInfoStart = 0x0035;
    static constexpr uint16_t kProductInfoCount = 20;
    static constexpr uint16_t kControllerBlockStart = 0x0100;
    static constexpr uint16_t kControllerBlockCount = 0x12;
    static constexpr uint16_t kFaultBlockStart = 0x0204;
    static constexpr uint16_t kFaultBlockCount = 4;
    static constexpr uint16_t kInverterBlockStart = 0x0210;
    static constexpr uint16_t kInverterBlockCount = 0x12;

    static bool readProductInfo(MODBUS_COM &mCom, char *buffer, size_t bufferSize);

    inline static const modbus_register_t registers_live[] = {
        {0x0100, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Battery_Percent, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0101, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_Voltage, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0102, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, "Battery_Current", 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0107, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV1_Input_Voltage, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0108, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV_Input_Current, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0109, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV1_Input_Power, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x010B, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Inverter_Charge_State, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x010E, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Charge_Power", 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x010F, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV2_Input_Voltage, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0110, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_PV2_Input_Current, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0111, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_PV2_Input_Power, 0, {}, nullptr, kControllerBlockStart, kControllerBlockCount},
        {0x0204, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Fault_Code, 0, {}, nullptr, kFaultBlockStart, kFaultBlockCount},
        {0x0210, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, DESCR_Inverter_Operation_Mode, 0, {.bitfield = {
                                                                                                                   "Power-up delay",
                                                                                                                   "Waiting",
                                                                                                                   "Initialization",
                                                                                                                   "Soft start",
                                                                                                                   "Mains",
                                                                                                                   "Inverter",
                                                                                                                   "Inverter to mains",
                                                                                                                   "Mains to inverter",
                                                                                                                   "Battery activate",
                                                                                                                   "Shutdown",
                                                                                                                   "Fault",
                                                                                                               }},
         nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0213, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_In_Voltage, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_TWO_DECIMAL, DESCR_AC_In_Frequency, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_AC_Out_Voltage, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0218, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_TWO_DECIMAL, DESCR_AC_Out_Frequency, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x021B, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_Watt, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x021C, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, DESCR_AC_Out_VA, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x021E, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Battery_Charge_Current, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x021F, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, DESCR_Output_Load_Percent, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0220, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_DCDC_Temperature, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
        {0x0221, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16_ONE_DECIMAL, DESCR_Inverter_Temperature, 0, {}, nullptr, kInverterBlockStart, kInverterBlockCount},
    };

    inline static const modbus_register_t registers_static[] = {
        {0, MODBUS_TYPE_HOLDING, REGISTER_TYPE_VIRTUAL_CALLBACK, DESCR_SRNE_PRODUCT_INFO, 0, {}, AnenjiSrne::productInfo},
    };
};

#endif
