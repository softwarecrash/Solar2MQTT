#pragma once

#include <Arduino.h>
#include <atomic>

class StatusLedService
{
public:
    void begin(int32_t pin, uint8_t brightness);
    void configure(int32_t pin, uint8_t brightness);
    void loop(bool wifiConnected, bool mqttEnabled, bool mqttConnected, bool inverterConnected);

private:
    static constexpr uint32_t kRepeatMs = 5000;
    static constexpr uint32_t kPulseSlotMs = 1000;
    static constexpr uint32_t kPulseFadeInMs = 250;
    static constexpr uint32_t kPulseHoldMs = 300;
    static constexpr uint32_t kPulseFadeOutMs = 250;
    static constexpr uint32_t kTaskIntervalMs = 20;
    static constexpr uint32_t kPwmFreqHz = 5000;
    static constexpr uint8_t kPwmResolutionBits = 8;

    std::atomic<int32_t> _desiredPin {-1};
    std::atomic<uint8_t> _desiredBrightness {0};
    std::atomic<bool> _wifiConnected {false};
    std::atomic<bool> _mqttEnabled {false};
    std::atomic<bool> _mqttConnected {false};
    std::atomic<bool> _inverterConnected {false};

    TaskHandle_t _taskHandle = nullptr;
    int32_t _activePin = -1;
    uint8_t _activeBrightness = 0;
    uint32_t _cycleStartMs = 0;
    uint8_t _cyclePulses = 0;
    bool _attached = false;

    uint8_t determinePulseCount(bool wifiConnected, bool mqttEnabled, bool mqttConnected, bool inverterConnected) const;
    static void taskEntry(void* arg);
    void taskLoop();
    void applyConfiguration();
    void attachPin();
    void detachPin();
    void writeBrightness(uint8_t value);
};
