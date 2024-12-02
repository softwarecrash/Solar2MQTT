#ifndef MODBUS_POW_HVM_LIP_H
#define MODBUS_POW_HVM_LIP_H

#include <modbus/device/modbus_device.h>
 

class Pow_Hvm_Lip : public ModbusDevice
{
public:
    Pow_Hvm_Lip() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {
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
    static const protocol_type_t _protocol = MODBUS_Pow_HVM_LIP;
    inline static const char *const _name = "PowMr POW-HVM-LIP";


    inline static const modbus_register_t registers_live[] = {
       {202, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_AC_In_Voltage},
       {203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, DESCR_AC_In_Frequenz},
       {219, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_PV_Input_Voltage},
    };

 
    inline static const modbus_register_t registers_static[] = { 
         {301, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, DESCR_Output_Source_Priority, 0, {.bitfield = {
                                                                                                         "Utility first (USB)",
                                                                                                         "Solar first (SUB)",
                                                                                                         "SBU priority",
                                                                                                     }}},
    };
    
};

#endif
