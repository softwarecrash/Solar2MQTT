#pragma once

#ifndef PIN_LED_STATUS
#error "PIN_LED_STATUS must be defined via build flags"
#endif

#ifndef PIN_INVERTER_RX_DEFAULT
#error "PIN_INVERTER_RX_DEFAULT must be defined via build flags"
#endif

#ifndef PIN_INVERTER_TX_DEFAULT
#error "PIN_INVERTER_TX_DEFAULT must be defined via build flags"
#endif

#ifndef PIN_INVERTER_DE_DEFAULT
#define PIN_INVERTER_DE_DEFAULT -1
#endif

#ifndef PIN_DS18B20
#define PIN_DS18B20 -1
#endif

#ifndef PIN_WS
#define PIN_WS -1
#endif

// D1 mini replacement layout:
// - ESP8266 D6 -> inverter RX
// - ESP8266 D7 -> inverter TX
// - ESP8266 D5 -> RS485 DE/RE
// - ESP8266 D2 -> DS18B20
// - ESP8266 D4 -> status LED
