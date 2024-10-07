#ifndef MODBUS_DEYE_H
#define MODBUS_DEYE_H

#include "modbus_device.h" 
 
class Deye : public ModbusDevice
{
public:
    Deye() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

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
    static const modbus_register_t registers_device_serial[];
};

#endif
