#include "anenji.h"


const modbus_register_t *Anenji::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *Anenji::getStaticRegisters() const
{
    return registers_static;
}

const char *Anenji::getName() const
{
    return _name;
}

bool Anenji::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
    uint16_t raw_value = 0;
    String serialCode = "";
    if (mCom.getModbusValue(202, MODBUS_TYPE_HOLDING, &raw_value, 1)){ 
        if ( mCom.getModbusValue(186, MODBUS_TYPE_HOLDING, &raw_value, 12)){
            writeLog("got serial\n"); 
            uint16_t registers[12];
            for (int i = 0; i < 12; ++i) {
                registers[i] = mCom.getModbusMaster()->getResponseBuffer(i);
            }

            serialCode = mCom.convertRegistersToASCII(registers, 12);
            Serial.println("Serial Code: " + serialCode);
        }
        snprintf(modelBuffer, bufferSize, "%s %s", getName(), serialCode.c_str());
        return true;
    }
    return false;
}

// Define the size calculation after the arrays are defined
size_t Anenji::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t Anenji::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}