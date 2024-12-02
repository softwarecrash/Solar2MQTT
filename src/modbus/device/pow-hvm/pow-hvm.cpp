#include "pow-hvm.h"

const modbus_register_t *Pow_Hvm::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *Pow_Hvm::getStaticRegisters() const
{
    return registers_static;
}

const char *Pow_Hvm::getName() const
{
    return _name;
}
bool Pow_Hvm::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
     uint16_t raw_value = 0;

    if (mCom.getModbusValue(4501, MODBUS_TYPE_HOLDING, &raw_value, 1)){
        snprintf(modelBuffer, bufferSize, "%s", getName());
        return true;
    }
 
    return false;
}

// Define the size calculation after the arrays are defined
size_t Pow_Hvm::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t Pow_Hvm::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}
 