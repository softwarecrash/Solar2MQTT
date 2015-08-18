#ifndef THINGSPEAK_H
#define THINGSPEAK_H

bool updateThingspeak(const char* apiKey, String& params);
void serviceThingspeak();

void updateThingspeakChargeApi();
void updateThingspeakBatteryApi();
void updateThingspeakLoadApi();

#endif
