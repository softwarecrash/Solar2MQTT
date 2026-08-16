#pragma once

#include <Arduino.h>

enum protocol_type_t
{
    NoD,
    PI18,
    PI30,
    MODBUS_MUST,
    MODBUS_DEYE,
    MODBUS_ANENJI,
    PI15,
    PI16,
    PI30_MAX,
    PI30_REVO,
    PI30_PIP_GK,
    PI41,
    PI30_UNKNOWN,
    MODBUS_SMG,
    MODBUS_SMG_II_11KW,
    MODBUS_ANENJI_SRNE,
    MODBUS_POWMR,
    PROTOCOL_TYPE_MAX
};

inline const char *const protocolStrings[] = {
    "NoD",
    "PI18",
    "PI30",
    "MODBUS_MUST",
    "MODBUS_DEYE",
    "MODBUS_ANENJI",
    "PI15",
    "PI16",
    "PI30_MAX",
    "PI30_REVO",
    "PI30_PIP_GK",
    "PI41",
    "PI30_UNKNOWN",
    "MODBUS_SMG",
    "MODBUS_SMG_II_11KW",
    "MODBUS_ANENJI_SRNE",
    "MODBUS_POWMR",
};

inline const char *protocolToString(protocol_type_t protocol)
{
    const size_t index = static_cast<size_t>(protocol);
    const size_t stringCount = sizeof(protocolStrings) / sizeof(protocolStrings[0]);
    if (index >= stringCount || protocolStrings[index] == nullptr)
    {
        return protocolStrings[0];
    }
    return protocolStrings[index];
}

inline bool protocolFromString(const char *value, protocol_type_t &protocol)
{
    if (value == nullptr)
    {
        return false;
    }

    const size_t stringCount = sizeof(protocolStrings) / sizeof(protocolStrings[0]);
    for (size_t i = 1; i < stringCount; ++i)
    {
        if (strcmp(value, protocolStrings[i]) == 0)
        {
            protocol = static_cast<protocol_type_t>(i);
            return true;
        }
    }
    return false;
}

inline bool isModbusProtocol(protocol_type_t protocol)
{
    return protocol == MODBUS_MUST || protocol == MODBUS_DEYE || protocol == MODBUS_ANENJI || protocol == MODBUS_SMG || protocol == MODBUS_SMG_II_11KW || protocol == MODBUS_ANENJI_SRNE || protocol == MODBUS_POWMR;
}

inline bool isPi30LikeProtocol(protocol_type_t protocol)
{
    return protocol == PI30 ||
           protocol == PI30_MAX ||
           protocol == PI30_REVO ||
           protocol == PI30_PIP_GK ||
           protocol == PI41 ||
           protocol == PI30_UNKNOWN;
}

inline bool isClassicPiStatusProtocol(protocol_type_t protocol)
{
    return protocol == PI16 || isPi30LikeProtocol(protocol);
}

inline bool isRawOnlyPiProtocol(protocol_type_t protocol)
{
    return protocol == PI15;
}

inline bool isStructuredPiProtocol(protocol_type_t protocol)
{
    return protocol == PI18 || isClassicPiStatusProtocol(protocol);
}

inline bool isAnyPiProtocol(protocol_type_t protocol)
{
    return isRawOnlyPiProtocol(protocol) || isStructuredPiProtocol(protocol);
}
