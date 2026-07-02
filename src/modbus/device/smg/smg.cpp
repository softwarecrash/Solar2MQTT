#include "smg.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace
{
bool isPrintableSerialChar(char value)
{
    return isalnum(static_cast<unsigned char>(value)) || value == '-' || value == '_' || value == '/' || value == '.';
}

void appendSerialChar(char value, char *buffer, size_t bufferSize, size_t &position)
{
    if (buffer == nullptr || bufferSize == 0 || position + 1 >= bufferSize)
    {
        return;
    }

    if (value == '\0' || value == static_cast<char>(0xFF))
    {
        return;
    }

    if (!isPrintableSerialChar(value))
    {
        return;
    }

    buffer[position++] = value;
    buffer[position] = '\0';
}

bool decodeSerial(const uint16_t *registers, size_t registerCount, char *buffer, size_t bufferSize)
{
    if (registers == nullptr || buffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    buffer[0] = '\0';
    size_t position = 0;
    for (size_t i = 0; i < registerCount; ++i)
    {
        appendSerialChar(static_cast<char>((registers[i] >> 8) & 0xFF), buffer, bufferSize, position);
        appendSerialChar(static_cast<char>(registers[i] & 0xFF), buffer, bufferSize, position);
    }

    return position >= 6;
}

bool valueInRange(uint16_t value, uint16_t minValue, uint16_t maxValue)
{
    return value >= minValue && value <= maxValue;
}
} // namespace

const modbus_register_t *SMG::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *SMG::getStaticRegisters() const
{
    return registers_static;
}

const char *SMG::getName() const
{
    return _name;
}

bool SMG::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
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

    char serial[32] = {};
    if (!decodeSerial(serialBlock, kSerialBlockCount, serial, sizeof(serial)))
    {
        return false;
    }

    uint16_t liveBlock[kLiveBlockCount] = {};
    if (!mCom.readHoldingBlock(kLiveBlockStart, kLiveBlockCount, liveBlock, kLiveBlockCount))
    {
        return false;
    }

    const uint16_t mode = liveBlock[201 - kLiveBlockStart];
    const uint16_t batteryVoltage = liveBlock[215 - kLiveBlockStart];
    const uint16_t batteryPercent = liveBlock[229 - kLiveBlockStart];
    const uint16_t loadPercent = liveBlock[225 - kLiveBlockStart];
    const uint16_t inverterTemp = liveBlock[227 - kLiveBlockStart];

    if (!valueInRange(mode, 0, 6) ||
        !valueInRange(batteryPercent, 0, 100) ||
        !valueInRange(loadPercent, 0, 100) ||
        !valueInRange(inverterTemp, 0, 150) ||
        !valueInRange(batteryVoltage, 50, 800))
    {
        return false;
    }

    snprintf(modelBuffer, bufferSize, "SMG %s", serial);
    return true;
}

size_t SMG::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t SMG::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}
