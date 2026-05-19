#include "smg_ii_11kw.h"

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

const modbus_register_t *SMGII11KW::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *SMGII11KW::getStaticRegisters() const
{
    return registers_static;
}

const char *SMGII11KW::getName() const
{
    return _name;
}

bool SMGII11KW::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
    if (modelBuffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    modelBuffer[0] = '\0';

    uint16_t protocolNumber = 0;
    if (!mCom.readHoldingBlock(kProtocolRegister, 1, &protocolNumber, 1) ||
        !valueInRange(protocolNumber, 3, 6))
    {
        return false;
    }

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

    uint16_t modeBlock[kModeBlockCount] = {};
    if (!mCom.readHoldingBlock(kModeBlockStart, kModeBlockCount, modeBlock, kModeBlockCount))
    {
        return false;
    }

    uint16_t batteryBlock[kBatteryBlockCount] = {};
    if (!mCom.readHoldingBlock(kBatteryBlockStart, kBatteryBlockCount, batteryBlock, kBatteryBlockCount))
    {
        return false;
    }

    const uint16_t mode = modeBlock[201 - kModeBlockStart];
    const uint16_t batteryVoltage = batteryBlock[277 - kBatteryBlockStart];
    const uint16_t batteryPercent = batteryBlock[280 - kBatteryBlockStart];

    if (!valueInRange(mode, 0, 6) ||
        !valueInRange(batteryVoltage, 300, 700) ||
        !valueInRange(batteryPercent, 0, 100))
    {
        return false;
    }

    snprintf(modelBuffer, bufferSize, "SMG-II 11KW %s", serial);
    return true;
}

size_t SMGII11KW::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t SMGII11KW::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}
