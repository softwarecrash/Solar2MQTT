#include "anenji_srne.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace
{
bool appendProductChar(char value, char *buffer, size_t bufferSize, size_t &position)
{
    if (buffer == nullptr || bufferSize == 0 || position + 1 >= bufferSize)
    {
        return false;
    }

    if (value == '\0' || value == static_cast<char>(0xFF) || !isprint(static_cast<unsigned char>(value)))
    {
        return false;
    }

    buffer[position++] = value;
    buffer[position] = '\0';
    return true;
}

bool decodeProductInfo(const uint16_t *registers, size_t registerCount, char *buffer, size_t bufferSize)
{
    if (registers == nullptr || buffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    buffer[0] = '\0';
    size_t position = 0;
    for (size_t i = 0; i < registerCount; ++i)
    {
        appendProductChar(static_cast<char>(registers[i] & 0xFF), buffer, bufferSize, position);
    }

    while (position > 0 && buffer[position - 1] == ' ')
    {
        buffer[--position] = '\0';
    }

    return position >= 3;
}

bool looksLikeSrneProductInfo(const char *productInfo)
{
    return productInfo != nullptr && strstr(productInfo, "SR") != nullptr;
}
} // namespace

const modbus_register_t *AnenjiSrne::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *AnenjiSrne::getStaticRegisters() const
{
    return registers_static;
}

const char *AnenjiSrne::getName() const
{
    return _name;
}

bool AnenjiSrne::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
    if (modelBuffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    modelBuffer[0] = '\0';

    char productInfo[32] = {};
    if (!readProductInfo(mCom, productInfo, sizeof(productInfo)) || !looksLikeSrneProductInfo(productInfo))
    {
        return false;
    }

    snprintf(modelBuffer, bufferSize, "Anenji SRNE %s", productInfo);
    return true;
}

size_t AnenjiSrne::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t AnenjiSrne::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}

void AnenjiSrne::productInfo(JsonObject *variant, uint16_t *registerValue, const modbus_register_t *reg, MODBUS_COM &mCom)
{
    (void)registerValue;

    if (variant == nullptr || reg == nullptr)
    {
        return;
    }

    char productInfo[32] = {};
    if (readProductInfo(mCom, productInfo, sizeof(productInfo)))
    {
        (*variant)[reg->name] = productInfo;
    }
}

bool AnenjiSrne::readProductInfo(MODBUS_COM &mCom, char *buffer, size_t bufferSize)
{
    uint16_t productBlock[kProductInfoCount] = {};
    if (!mCom.readHoldingBlock(kProductInfoStart, kProductInfoCount, productBlock, kProductInfoCount))
    {
        return false;
    }

    return decodeProductInfo(productBlock, kProductInfoCount, buffer, bufferSize);
}
