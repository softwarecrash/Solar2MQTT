#ifndef MODBUS_DEVICE_H
#define MODBUS_DEVICE_H

#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <main.h>
#include <modbus/modbus_registers.h>
#include <modbus/modbus.h>

class MODBUS; // Forward declaration of the MODBUS class

class ModbusDevice
{
public:
    ModbusDevice(long baudRate, uint32_t modbusAddr, protocol_type_t protocol) : _baudRate(baudRate), _modbusAddr(modbusAddr), _protocol(protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const = 0;
    virtual const modbus_register_t *getStaticRegisters() const = 0;

    virtual long getBaudRate()
    {
        return _baudRate;
    }

    virtual long getModbusAddr()
    {
        return _modbusAddr;
    }

    virtual protocol_type_t getProtocol()
    {
        return _protocol;
    }

    virtual const char *getName() const = 0; // Pure virtual function

    virtual void init(SoftwareSerial &serial, ModbusMaster &mb)
    {
        serial.begin(getBaudRate(), SWSERIAL_8N1);
        mb.begin(_modbusAddr, serial);
    }

    virtual bool retrieveModel(MODBUS &modbus, char *modelBuffer, size_t bufferSize)
    {
        modelBuffer[0] = '\0';
        return false;
    }

    virtual ~ModbusDevice() {}

protected:
    long _baudRate;
    uint32_t _modbusAddr;
    protocol_type_t _protocol;
};

#endif