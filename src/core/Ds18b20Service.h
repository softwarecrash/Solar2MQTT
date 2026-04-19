#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MycilaDS18.h>

#include <memory>
#include <vector>

class Ds18b20Service
{
public:
    Ds18b20Service();

    void begin(uint8_t pin, JsonObject target);
    void loop();
    size_t sensorCount() const { return _sensorCount; }

    void setCallback(std::function<void(uint8_t, float)> callback) { _callback = callback; }

private:
    struct SensorSlot
    {
        uint64_t address = 0;
        std::unique_ptr<Mycila::DS18> sensor;
        bool hasValue = false;
        float lastTemperature = 0.0f;
        uint8_t missedPolls = 0;
    };

    bool discoverSensors();
    void clearTargetRange(size_t fromIndex);
    void publishValue(size_t index, float temperature);
    void clearValue(size_t index);

    std::unique_ptr<OneWire32> _oneWire;
    std::vector<SensorSlot> _sensors;
    JsonObject _target;
    std::function<void(uint8_t, float)> _callback;
    int8_t _pin;
    uint32_t _nextPollAt;
    uint32_t _nextDiscoveryAt;
    uint8_t _consecutiveFailedPolls;
    bool _enabled;
    bool _active;
    size_t _sensorCount;
};
