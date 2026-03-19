#include "core/Ds18b20Service.h"

#include "core/LogSerial.h"

Ds18b20Service *Ds18b20Service::s_instance = nullptr;

Ds18b20Service::Ds18b20Service()
    : _oneWire(0), _dallas(&_oneWire), _nonBlocking(&_dallas), _active(false), _sensorCount(0)
{
}

void Ds18b20Service::begin(uint8_t pin, JsonObject target)
{
    if (pin == static_cast<uint8_t>(-1))
    {
        _active = false;
        return;
    }

    _oneWire = OneWire(pin);
    _dallas = DallasTemperature(&_oneWire);
    _nonBlocking = NonBlockingDallas(&_dallas);
    _target = target;
    _active = true;
    s_instance = this;

    _dallas.begin();
    _nonBlocking.begin(NonBlockingDallas::resolution_12, 1500);
    _sensorCount = _nonBlocking.getSensorsCount();
    LogSerial.printf("[DS18B20] Sensors found: %u\n", static_cast<unsigned>(_sensorCount));

    _nonBlocking.onTemperatureChange([](int deviceIndex, int32_t temperatureRaw)
                                     {
        if (!s_instance)
        {
            return;
        }

        const float temperatureC = s_instance->_nonBlocking.rawToCelsius(temperatureRaw);
        char key[16];
        snprintf(key, sizeof(key), "DS18B20_%u", static_cast<unsigned>(deviceIndex + 1));
        s_instance->_target[key] = temperatureC;
        if (s_instance->_callback)
        {
            s_instance->_callback(static_cast<uint8_t>(deviceIndex + 1), temperatureC);
        } });

    _nonBlocking.onDeviceDisconnected([](int deviceIndex)
                                      {
        if (!s_instance)
        {
            return;
        }

        char key[16];
        snprintf(key, sizeof(key), "DS18B20_%u", static_cast<unsigned>(deviceIndex + 1));
        s_instance->_target[key] = nullptr; });

    delay(100);
    _nonBlocking.requestTemperature();
}

void Ds18b20Service::loop()
{
    if (_active)
    {
        _nonBlocking.update();
    }
}
