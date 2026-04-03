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
};

inline const char *protocolToString(protocol_type_t protocol)
{
    const size_t index = static_cast<size_t>(protocol);
    if (index >= static_cast<size_t>(PROTOCOL_TYPE_MAX))
    {
        return protocolStrings[0];
    }
    return protocolStrings[index];
}

inline bool isModbusProtocol(protocol_type_t protocol)
{
    return protocol == MODBUS_MUST || protocol == MODBUS_DEYE || protocol == MODBUS_ANENJI || protocol == MODBUS_SMG;
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

inline bool isRawOnlyPiProtocol(protocol_type_t protocol)
{
    return protocol == PI15 || protocol == PI16;
}

inline bool isStructuredPiProtocol(protocol_type_t protocol)
{
    return protocol == PI18 || isPi30LikeProtocol(protocol);
}

inline bool isAnyPiProtocol(protocol_type_t protocol)
{
    return isRawOnlyPiProtocol(protocol) || isStructuredPiProtocol(protocol);
}
