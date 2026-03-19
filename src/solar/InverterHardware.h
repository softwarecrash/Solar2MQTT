#pragma once

#include <Arduino.h>

struct InverterHardwareConfig
{
    int rxPin;
    int txPin;
    int dirPin;
};

extern InverterHardwareConfig g_inverterHardwareConfig;
