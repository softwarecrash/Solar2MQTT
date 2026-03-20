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

#ifndef HAS_LAN
#error "HAS_LAN must be defined via build flags"
#endif

#if HAS_LAN
#if defined(ETH_SPI_W5500) && ETH_SPI_W5500
#ifndef PIN_ETH_SPI_MISO
#error "PIN_ETH_SPI_MISO must be defined via build flags when ETH_SPI_W5500=1"
#endif
#ifndef PIN_ETH_SPI_MOSI
#error "PIN_ETH_SPI_MOSI must be defined via build flags when ETH_SPI_W5500=1"
#endif
#ifndef PIN_ETH_SPI_SCLK
#error "PIN_ETH_SPI_SCLK must be defined via build flags when ETH_SPI_W5500=1"
#endif
#ifndef PIN_ETH_SPI_CS
#error "PIN_ETH_SPI_CS must be defined via build flags when ETH_SPI_W5500=1"
#endif
#ifndef PIN_ETH_SPI_RST
#error "PIN_ETH_SPI_RST must be defined via build flags when ETH_SPI_W5500=1"
#endif
#ifndef PIN_ETH_SPI_INT
#error "PIN_ETH_SPI_INT must be defined via build flags when ETH_SPI_W5500=1"
#endif
#else
#ifndef PIN_ETH_POWER
#error "PIN_ETH_POWER must be defined via build flags when HAS_LAN=1"
#endif
#ifndef PIN_ETH_MDC
#error "PIN_ETH_MDC must be defined via build flags when HAS_LAN=1"
#endif
#ifndef PIN_ETH_MDIO
#error "PIN_ETH_MDIO must be defined via build flags when HAS_LAN=1"
#endif
#ifndef PIN_ETH_PHY_ADDR
#error "PIN_ETH_PHY_ADDR must be defined via build flags when HAS_LAN=1"
#endif
#endif
#endif

// D1 mini replacement layout:
// - ESP8266 D6 -> inverter RX
// - ESP8266 D7 -> inverter TX
// - ESP8266 D5 -> RS485 DE/RE
// - ESP8266 D2 -> DS18B20
// - ESP8266 D4 -> status LED

// Waveshare ESP32-S3-ETH (W5500 SPI) defaults used in this project:
// - MISO GPIO12
// - MOSI GPIO11
// - SCLK GPIO13
// - CS   GPIO14
// - RST  GPIO9
// - INT  GPIO10
