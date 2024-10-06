#ifdef ONE_WIRE_BUS

#include "DSTemp.h"

extern void writeLog(const char *format, ...);

DSTemp::DSTemp() {
    oneWire = OneWire(ONE_WIRE_BUS);
    sensors = DallasTemperature(&oneWire);
}

void DSTemp::init() {
    sensors.begin();
    if (sensors.getDeviceCount() > 0)
    {
        if (!sensors.getAddress(address, 0))
        {
            writeLog("Dallas temperature sensor isn't found");
            return;
        }
        sensors.setResolution(address, 9);

        isSensorExist = true;
        lastUpdateTime = 0;

        writeLog("Dallas temperature sensor found");
    }
    isInit = true;
}

void DSTemp::loop() {
    if (!isInit)
    {
        init();
    }
    if (isInit && isSensorExist)
    {
        if (millis() - lastUpdateTime > 5000)
        {
            lastUpdateTime = millis();
            sensors.requestTemperatures();
            float tempC = sensors.getTempC(address);
            if (tempC == DEVICE_DISCONNECTED_C)
            {
                writeLog("Error: Could not read temperature data");
                return;
            }
            char tmp[8];
            sprintf(tmp, "%.1f", tempC);
            liveData["External_Temperature"] = tmp;
        }
    }
}

#endif