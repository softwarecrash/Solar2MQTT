#include "core/InternalTemperatureService.h"

#include "core/LogSerial.h"
#include "descriptors.h"

#include <math.h>
#include "soc/soc_caps.h"

namespace
{
constexpr uint32_t kPollIntervalMs = 10000;
constexpr float kChangeThresholdC = 0.1f;

#if defined(CONFIG_IDF_TARGET_ESP32) || (defined(SOC_TEMP_SENSOR_SUPPORTED) && SOC_TEMP_SENSOR_SUPPORTED)
constexpr bool kInternalTemperatureSupported = true;
#else
constexpr bool kInternalTemperatureSupported = false;
#endif

bool timeReached(uint32_t now, uint32_t scheduledAt)
{
    return static_cast<int32_t>(now - scheduledAt) >= 0;
}

bool nearlyEqual(float lhs, float rhs)
{
    return fabsf(lhs - rhs) < kChangeThresholdC;
}

bool isValidTemperature(float value)
{
    return isfinite(value) && value > -100.0f && value < 150.0f;
}

double roundedTemperature(float value)
{
    return roundf(value * 100.0f) / 100.0f;
}

float readInternalTemperature()
{
#if defined(CONFIG_IDF_TARGET_ESP32) || (defined(SOC_TEMP_SENSOR_SUPPORTED) && SOC_TEMP_SENSOR_SUPPORTED)
    return temperatureRead();
#else
    return NAN;
#endif
}
} // namespace

InternalTemperatureService::InternalTemperatureService()
    : _nextPollAt(0),
      _lastTemperature(NAN),
      _supported(kInternalTemperatureSupported),
      _hasValue(false)
{
}

void InternalTemperatureService::begin(JsonObject target)
{
    _target = target;
    _nextPollAt = 0;
    _lastTemperature = NAN;
    _hasValue = false;
    _supported = kInternalTemperatureSupported;

    if (!_supported)
    {
        if (!_target.isNull())
        {
            _target.remove(DESCR_ESP_Internal_Temperature);
        }
        LogSerial.println("[ESPTemp] Internal temperature sensor not supported on this build target");
        return;
    }

    LogSerial.println("[ESPTemp] Internal temperature sensor enabled");
    poll(false);
    _nextPollAt = millis() + kPollIntervalMs;
}

void InternalTemperatureService::loop()
{
    if (!_supported)
    {
        return;
    }

    const uint32_t now = millis();
    if (!timeReached(now, _nextPollAt))
    {
        return;
    }

    poll(true);
    _nextPollAt = now + kPollIntervalMs;
}

void InternalTemperatureService::poll(bool notify)
{
    if (_target.isNull())
    {
        return;
    }

    const float temperature = readInternalTemperature();
    if (!isValidTemperature(temperature))
    {
        if (_hasValue)
        {
            clearValue(notify);
        }
        return;
    }

    if (_hasValue && nearlyEqual(_lastTemperature, temperature))
    {
        return;
    }

    _lastTemperature = temperature;
    _hasValue = true;
    _target[DESCR_ESP_Internal_Temperature] = roundedTemperature(temperature);

    if (notify && _callback)
    {
        _callback();
    }
}

void InternalTemperatureService::clearValue(bool notify)
{
    if (_target.isNull())
    {
        return;
    }

    _target[DESCR_ESP_Internal_Temperature] = nullptr;
    _hasValue = false;
    _lastTemperature = NAN;

    if (notify && _callback)
    {
        _callback();
    }
}
