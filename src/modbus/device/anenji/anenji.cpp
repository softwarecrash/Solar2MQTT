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
    if (modelBuffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    modelBuffer[0] = '\0';

    uint16_t serialBlock[kSerialBlockCount] = {};
    if (!mCom.readHoldingBlock(kSerialBlockStart, kSerialBlockCount, serialBlock, kSerialBlockCount))
    {
        return false;
    }

    size_t position = 0;
    for (size_t i = 0; i < kSerialBlockCount && position + 1 < bufferSize; ++i)
    {
        char highByte = static_cast<char>((serialBlock[i] >> 8) & 0xFF);
        char lowByte = static_cast<char>(serialBlock[i] & 0xFF);

        if (highByte != '\0' && highByte != static_cast<char>(0xFF) && position + 1 < bufferSize)
        {
            modelBuffer[position++] = highByte;
        }
        if (lowByte != '\0' && lowByte != static_cast<char>(0xFF) && position + 1 < bufferSize)
        {
            modelBuffer[position++] = lowByte;
        }
    }
    modelBuffer[position] = '\0';

    return position > 0;
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
