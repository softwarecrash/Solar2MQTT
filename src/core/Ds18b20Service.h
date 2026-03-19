#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <NonBlockingDallas.h>
#include <OneWire.h>

class Ds18b20Service
{
public:
    Ds18b20Service();

    void begin(uint8_t pin, JsonObject target);
    void loop();
    size_t sensorCount() const { return _sensorCount; }

    void setCallback(std::function<void(uint8_t, float)> callback) { _callback = callback; }

private:
    static Ds18b20Service *s_instance;

    OneWire _oneWire;
    DallasTemperature _dallas;
    NonBlockingDallas _nonBlocking;
    JsonObject _target;
    std::function<void(uint8_t, float)> _callback;
    bool _active;
    size_t _sensorCount;
};
