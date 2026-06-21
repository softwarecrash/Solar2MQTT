#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include <functional>

class InternalTemperatureService
{
public:
    InternalTemperatureService();

    void begin(JsonObject target);
    void loop();

    bool supported() const { return _supported; }
    bool hasValue() const { return _hasValue; }

    void setCallback(std::function<void()> callback) { _callback = callback; }

private:
    void poll(bool notify);
    void clearValue(bool notify);

    JsonObject _target;
    std::function<void()> _callback;
    uint32_t _nextPollAt;
    float _lastTemperature;
    bool _supported;
    bool _hasValue;
};
