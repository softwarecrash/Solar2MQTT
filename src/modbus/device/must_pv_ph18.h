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
    static const long _baudRate;
    static const uint32_t _modbusAddr;
    static const protocol_type_t _protocol;
    static const char *const _name;

    static const modbus_register_t registers_live[];
    static const modbus_register_t registers_static[];
    static const modbus_register_t registers_device_model[];
};

#endif
