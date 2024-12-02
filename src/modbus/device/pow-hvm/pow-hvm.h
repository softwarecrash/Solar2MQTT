#ifndef MODBUS_POW_HVM_H
#define MODBUS_POW_HVM_H

/*
    should work with devices on https://powmr.com/products/powmr-wifi-module-with-rs232-remote-monitoring-solution-wifi-vm dongle
*/

#include <modbus/device/modbus_device.h>
 

class Pow_Hvm : public ModbusDevice
{
public:
    Pow_Hvm() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {
        _dataFilter = [](uint16_t data) -> uint16_t {
            return htons(data); 
        };
    }

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;
    static void generationSum(JsonObject *variant, uint16_t *registerValue, const modbus_register_t *reg, MODBUS_COM &mCom);

private:
    static const long _baudRate = 2400;
    static const uint32_t _modbusAddr = 5;
    static const protocol_type_t _protocol = MODBUS_POW_HVM;
    inline static const char *const _name = "PowMr POW-HVM";


    inline static const modbus_register_t registers_live[] = {
       {4502, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_In_Voltage},
       {4503, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_In_Frequenz},
       {4504, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_PV_Input_Voltage},
    };

 
    inline static const modbus_register_t registers_static[] = { 
         {4537, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Output_Source_Priority, 0, {.bitfield = {
                                                                                                         "Utility first (USB)",
                                                                                                         "Solar first (SUB)",
                                                                                                         "SBU priority",
                                                                                                     }}},
    };
    
};

#endif
