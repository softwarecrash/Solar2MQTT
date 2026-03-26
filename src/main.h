#pragma once

#include <Arduino.h>

#include "solar/SolarTypes.h"

extern bool g_pendingRestart;
extern uint32_t g_restartAt;
extern bool g_pendingFactoryReset;
extern uint32_t g_factoryResetAt;
extern bool g_pendingNetworkReconfigure;
extern uint32_t g_networkReconfigureAt;
extern bool g_pendingInverterReconfigure;
extern uint32_t g_inverterReconfigureAt;

void writeLog(const char *format, ...);
