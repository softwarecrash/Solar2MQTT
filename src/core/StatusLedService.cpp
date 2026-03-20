#include "core/StatusLedService.h"

#include <algorithm>
#include <math.h>

#include "core/LogSerial.h"

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float smoothPulsePhase(uint32_t elapsedMs, uint32_t durationMs, bool rising)
{
    if (durationMs == 0)
    {
        return rising ? 1.0f : 0.0f;
    }

    const float phase = static_cast<float>(elapsedMs) / static_cast<float>(durationMs);
    const float eased = 0.5f - 0.5f * cosf(std::min(1.0f, std::max(0.0f, phase)) * kPi);
    return rising ? eased : (1.0f - eased);
}
} // namespace

void StatusLedService::begin(int32_t pin, uint8_t brightness)
{
    configure(pin, brightness);

    if (_taskHandle != nullptr)
    {
        return;
    }

    BaseType_t result = xTaskCreate(
        &StatusLedService::taskEntry,
        "status-led",
        3072,
        this,
        1,
        &_taskHandle);

    if (result != pdPASS)
    {
        _taskHandle = nullptr;
        LogSerial.println("[LED] Failed to start status LED task");
    }
}

void StatusLedService::configure(int32_t pin, uint8_t brightness)
{
    _desiredPin.store(pin, std::memory_order_relaxed);
    _desiredBrightness.store(brightness, std::memory_order_relaxed);
}

void StatusLedService::loop(bool wifiConnected, bool mqttEnabled, bool mqttConnected, bool inverterConnected)
{
    _wifiConnected.store(wifiConnected, std::memory_order_relaxed);
    _mqttEnabled.store(mqttEnabled, std::memory_order_relaxed);
    _mqttConnected.store(mqttConnected, std::memory_order_relaxed);
    _inverterConnected.store(inverterConnected, std::memory_order_relaxed);
}

uint8_t StatusLedService::determinePulseCount(bool wifiConnected, bool mqttEnabled, bool mqttConnected, bool inverterConnected) const
{
    if (!wifiConnected)
    {
        return 4;
    }

    if (mqttEnabled && !mqttConnected)
    {
        return 3;
    }

    if (!inverterConnected)
    {
        return 2;
    }

    return 1;
}

void StatusLedService::taskEntry(void* arg)
{
    static_cast<StatusLedService*>(arg)->taskLoop();
}

void StatusLedService::taskLoop()
{
    for (;;)
    {
        applyConfiguration();

        if (!_attached || _activePin < 0 || _activeBrightness == 0)
        {
            if (_attached)
            {
                writeBrightness(0);
            }
            vTaskDelay(pdMS_TO_TICKS(kTaskIntervalMs));
            continue;
        }

        const bool wifiConnected = _wifiConnected.load(std::memory_order_relaxed);
        const bool mqttEnabled = _mqttEnabled.load(std::memory_order_relaxed);
        const bool mqttConnected = _mqttConnected.load(std::memory_order_relaxed);
        const bool inverterConnected = _inverterConnected.load(std::memory_order_relaxed);

        const uint32_t now = millis();
        if (_cycleStartMs == 0)
        {
            _cycleStartMs = now;
            _cyclePulses = determinePulseCount(wifiConnected, mqttEnabled, mqttConnected, inverterConnected);
        }

        if ((now - _cycleStartMs) >= kRepeatMs)
        {
            const uint32_t elapsedCycles = (now - _cycleStartMs) / kRepeatMs;
            _cycleStartMs += elapsedCycles * kRepeatMs;
            _cyclePulses = determinePulseCount(wifiConnected, mqttEnabled, mqttConnected, inverterConnected);
        }

        if (_cyclePulses == 0)
        {
            writeBrightness(0);
            vTaskDelay(pdMS_TO_TICKS(kTaskIntervalMs));
            continue;
        }

        const uint32_t cycleElapsed = now - _cycleStartMs;
        const uint32_t activeWindowMs = static_cast<uint32_t>(_cyclePulses) * kPulseSlotMs;
        if (cycleElapsed >= activeWindowMs)
        {
            writeBrightness(0);
            vTaskDelay(pdMS_TO_TICKS(kTaskIntervalMs));
            continue;
        }

        const uint32_t slotElapsed = cycleElapsed % kPulseSlotMs;
        const uint32_t pulseActiveMs = kPulseFadeInMs + kPulseHoldMs + kPulseFadeOutMs;
        if (slotElapsed >= pulseActiveMs)
        {
            writeBrightness(0);
            vTaskDelay(pdMS_TO_TICKS(kTaskIntervalMs));
            continue;
        }

        float levelFactor = 0.0f;
        if (slotElapsed < kPulseFadeInMs)
        {
            levelFactor = smoothPulsePhase(slotElapsed, kPulseFadeInMs, true);
        }
        else if (slotElapsed < (kPulseFadeInMs + kPulseHoldMs))
        {
            levelFactor = 1.0f;
        }
        else
        {
            const uint32_t fadeOutElapsed = slotElapsed - kPulseFadeInMs - kPulseHoldMs;
            levelFactor = smoothPulsePhase(fadeOutElapsed, kPulseFadeOutMs, false);
        }

        const uint8_t level = static_cast<uint8_t>(lroundf(static_cast<float>(_activeBrightness) * levelFactor));
        writeBrightness(level);
        vTaskDelay(pdMS_TO_TICKS(kTaskIntervalMs));
    }
}

void StatusLedService::applyConfiguration()
{
    const int32_t desiredPin = _desiredPin.load(std::memory_order_relaxed);
    const uint8_t desiredBrightness = _desiredBrightness.load(std::memory_order_relaxed);

    if (desiredPin != _activePin)
    {
        detachPin();
        _activePin = desiredPin;
        _cycleStartMs = 0;
        _cyclePulses = 0;

        if (_activePin >= 0)
        {
            attachPin();
        }
    }

    _activeBrightness = desiredBrightness;
}

void StatusLedService::attachPin()
{
    if (_activePin < 0 || _attached)
    {
        return;
    }

    if (!ledcAttach(static_cast<uint8_t>(_activePin), kPwmFreqHz, kPwmResolutionBits))
    {
        LogSerial.printf("[LED] Failed to attach status LED pin %ld\n", static_cast<long>(_activePin));
        return;
    }

    _attached = true;
    writeBrightness(0);
}

void StatusLedService::detachPin()
{
    if (!_attached || _activePin < 0)
    {
        _attached = false;
        return;
    }

    writeBrightness(0);
    ledcDetach(static_cast<uint8_t>(_activePin));
    pinMode(static_cast<uint8_t>(_activePin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(_activePin), LOW);
    _attached = false;
}

void StatusLedService::writeBrightness(uint8_t value)
{
    if (!_attached || _activePin < 0)
    {
        return;
    }

    ledcWrite(static_cast<uint8_t>(_activePin), value);
}
