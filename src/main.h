#ifndef INVERTER_TX
#define INVERTER_TX 13
#endif
#ifndef INVERTER_RX
#define INVERTER_RX 12
#endif

bool sendtoMQTT();
void mqttcallback(char *top, unsigned char *payload, unsigned int length);