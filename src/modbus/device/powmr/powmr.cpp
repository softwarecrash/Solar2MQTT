#include "powmr.h"

namespace
{
uint16_t swapRegisterBytes(uint16_t value)
{
    return static_cast<uint16_t>((value >> 8) | (value << 8));
}

bool isZeroOrInRange(uint16_t value, uint16_t minimum, uint16_t maximum)
{
    return value == 0 || (value >= minimum && value <= maximum);
}
} // namespace

void PowMr::pollPv2(MODBUS_COM &mCom, JsonObject live)
{
    _pv2.poll(millis(),
        [&](uint16_t *words) {
            uint8_t result = 0;
            if (mCom.readHoldingBlockOnce(4563, 2, words, 2, PowMrPv2::kResponseTimeoutMs, &result))
                return PowMrPv2::ReadResult::Ok;
            if (result == ModbusMaster::ku8MBIllegalFunction || result == ModbusMaster::ku8MBIllegalDataAddress)
                return PowMrPv2::ReadResult::Unsupported;
            return PowMrPv2::ReadResult::Failed;
        },
        [&](float voltage, uint16_t power) {
            live[DESCR_PV2_Input_Voltage] = voltage;
            live[DESCR_PV2_Input_Power] = power;
            if (!live[DESCR_PV_Input_Power].isNull())
            {
                live[DESCR_PV_Total_Input_Power] = live[DESCR_PV_Input_Power].as<uint32_t>() + power;
            }
        },
        [&]() {
            live.remove(DESCR_PV2_Input_Voltage);
            live.remove(DESCR_PV2_Input_Power);
            live.remove(DESCR_PV_Total_Input_Power);
            writeLog("PowMr PV2 unavailable; optional probe suppressed or backed off");
        });
}

const modbus_register_t *PowMr::getLiveRegisters() const
{
    return registers_live;
}

const modbus_register_t *PowMr::getStaticRegisters() const
{
    return registers_static;
}

const char *PowMr::getName() const
{
    return _name;
}

bool PowMr::retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize)
{
    if (modelBuffer == nullptr || bufferSize == 0)
    {
        return false;
    }

    modelBuffer[0] = '\0';
    uint16_t block[kMainBlockCount] = {};
    if (!mCom.readHoldingBlock(kMainBlockStart, kMainBlockCount, block, kMainBlockCount))
    {
        return false;
    }

    const uint16_t gridVoltage = swapRegisterBytes(block[4502 - kMainBlockStart]);
    const uint16_t gridFrequency = swapRegisterBytes(block[4503 - kMainBlockStart]);
    const uint16_t batteryVoltage = swapRegisterBytes(block[4506 - kMainBlockStart]);
    const uint16_t batteryPercent = swapRegisterBytes(block[4507 - kMainBlockStart]);
    const uint16_t outputVoltage = swapRegisterBytes(block[4510 - kMainBlockStart]);
    const uint16_t outputFrequency = swapRegisterBytes(block[4511 - kMainBlockStart]);

    if (!isZeroOrInRange(gridVoltage, 800, 3000) ||
        !isZeroOrInRange(gridFrequency, 450, 650) ||
        batteryVoltage < 80 || batteryVoltage > 700 ||
        batteryPercent > 100 ||
        !isZeroOrInRange(outputVoltage, 800, 3000) ||
        !isZeroOrInRange(outputFrequency, 450, 650))
    {
        return false;
    }

    snprintf(modelBuffer, bufferSize, "PowMr HVM Modbus 4501");
    return true;
}

size_t PowMr::getLiveRegistersCount() const
{
    return sizeof(registers_live) / sizeof(modbus_register_t);
}

size_t PowMr::getStaticRegistersCount() const
{
    return sizeof(registers_static) / sizeof(modbus_register_t);
}
