#ifndef DALLAS_TEMPERATURE_H
#define DALLAS_TEMPERATURE_H
#ifdef ONE_WIRE_BUS

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

extern JsonObject liveData;

class DSTemp {
private:
    OneWire oneWire;
    DallasTemperature sensors;
    DeviceAddress address{};
    bool isInit = false;
    bool isSensorExist = false;
    uint32_t lastUpdateTime = 0;
    void init();
public:
    DSTemp();
    void loop();
};

#endif
#endif
