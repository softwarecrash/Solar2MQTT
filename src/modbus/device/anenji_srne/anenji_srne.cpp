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

uint16_t blockValue(const uint16_t *registers, uint16_t blockStart, uint16_t registerId)
{
    return registers[registerId - blockStart];
}

void setU16(JsonObject *variant, const char *name, uint16_t raw)
{
    (*variant)[name] = raw;
}

void setI16OneDecimal(JsonObject *variant, const char *name, uint16_t raw)
{
    (*variant)[name] = static_cast<int16_t>(raw) / 10.0f;
}

void setI16TwoDecimals(JsonObject *variant, const char *name, uint16_t raw)
{
    (*variant)[name] = static_cast<int16_t>(raw) / 100.0f;
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

    _phaseBlockAvailable = true;
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

void AnenjiSrne::extendedInverterData(JsonObject *variant, uint16_t *registerValue, const modbus_register_t *reg, MODBUS_COM &mCom)
{
    (void)registerValue;
    (void)reg;

    if (variant == nullptr)
    {
        return;
    }

    uint16_t inverterBlock[kInverterBlockCount] = {};
    if (mCom.readHoldingBlock(kInverterBlockStart, kInverterBlockCount, inverterBlock, kInverterBlockCount))
    {
        setI16OneDecimal(variant, DESCR_AC_In_Voltage_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0213));
        setI16OneDecimal(variant, DESCR_AC_In_Current_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0214));
        setI16TwoDecimals(variant, DESCR_AC_In_Frequency_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0215));
        setI16OneDecimal(variant, DESCR_AC_Out_Voltage_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0216));
        setI16TwoDecimals(variant, DESCR_AC_Out_Frequency_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0218));
        setI16OneDecimal(variant, DESCR_AC_Out_Current_L1, blockValue(inverterBlock, kInverterBlockStart, 0x0219));
        setI16OneDecimal(variant, DESCR_AC_Output_Current, blockValue(inverterBlock, kInverterBlockStart, 0x0219));
        setU16(variant, DESCR_AC_Out_Watt_L1, blockValue(inverterBlock, kInverterBlockStart, 0x021B));
        setU16(variant, DESCR_AC_Out_VA_L1, blockValue(inverterBlock, kInverterBlockStart, 0x021C));
        setU16(variant, DESCR_AC_Out_Percent_L1, blockValue(inverterBlock, kInverterBlockStart, 0x021F));
    }

    if (_phaseBlockAvailable)
    {
        uint16_t phaseBlock[kPhaseBlockCount] = {};
        if (mCom.readHoldingBlock(kPhaseBlockStart, kPhaseBlockCount, phaseBlock, kPhaseBlockCount))
        {
            setI16OneDecimal(variant, DESCR_AC_In_Voltage_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x022A));
            setI16OneDecimal(variant, DESCR_AC_In_Voltage_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x022B));
            setI16OneDecimal(variant, DESCR_AC_Out_Voltage_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x022C));
            setI16OneDecimal(variant, DESCR_AC_Out_Voltage_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x022D));
            setI16OneDecimal(variant, DESCR_AC_Out_Current_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x0230));
            setI16OneDecimal(variant, DESCR_AC_Out_Current_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x0231));
            setU16(variant, DESCR_AC_Out_Watt_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x0232));
            setU16(variant, DESCR_AC_Out_Watt_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x0233));
            setU16(variant, DESCR_AC_Out_VA_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x0234));
            setU16(variant, DESCR_AC_Out_VA_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x0235));
            setU16(variant, DESCR_AC_Out_Percent_L2, blockValue(phaseBlock, kPhaseBlockStart, 0x0236));
            setU16(variant, DESCR_AC_Out_Percent_L3, blockValue(phaseBlock, kPhaseBlockStart, 0x0237));
        }
        else
        {
            _phaseBlockAvailable = false;
        }
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
