#include "modbus_device.h"
// Constructor definition
ModbusDevice::ModbusDevice(long baudRate, uint32_t modbusAddr, protocol_type_t protocol)
    : _baudRate(baudRate), _modbusAddr(modbusAddr), _protocol(protocol) {}

// Method implementations for non-virtual functions
long ModbusDevice::getBaudRate()
{
    return _baudRate;
}

long ModbusDevice::getModbusAddr()
{
    return _modbusAddr;
}

protocol_type_t ModbusDevice::getProtocol()
{
    return _protocol;
}

void ModbusDevice::init(SoftwareSerial &serial, MODBUS_COM &mCom)
{
    serial.begin(getBaudRate(), SWSERIAL_8N1);
    mCom.getModbusMaster().begin(_modbusAddr, serial);
}

bool ModbusDevice::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
    modelBuffer[0] = '\0';
    return false;
}

// Destructor definition
ModbusDevice::~ModbusDevice() {}