#pragma once

#include <Arduino.h>

class FactoryResetManager
{
public:
    static void begin(uint32_t clearTimeoutMs = 15000, uint8_t pressesRequired = 6);
    static void loop();

private:
    static void handleBootResetCount();
    static void clearResetCounter();
    static void performFactoryReset();
};
