#pragma once
#include <Arduino.h>
#include "BatteryStorageMock.h"
#include <cassert>
#include <cmath>
#include "descriptors.h"
#define private public
#include "core/SolarState.h"
#include "core/EnergyBacklog.h"
#undef private

namespace battery_test
{
Storage storage;
}
FakeLittleFS LittleFS;
uint32_t testNow = 100;
FakeEsp ESP;
void writeLog(const char *, ...) {}
static void near(double actual, double expected)
{
    assert(std::isfinite(actual) && std::isfinite(expected));
    if (std::fabs(actual - expected) > 0.000001)
    {
        std::fprintf(stderr, "%.9f != %.9f\n", actual, expected);
        std::abort();
    }
}
