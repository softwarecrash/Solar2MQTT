# Features:
- WiFi Ap with setup
- Read and publish the QPIGS, QPIRI QMOD and more....
- Web display Data for Inverters where support the QPIGS Request
- Web Update, Reset, Restart, Configure
- send Data over MQTT
- Building using Platform.io
- Live web pages with ajax

# Last Added:
- fix some Wsign Compiler Warnings
- fix answer string from inverter and remove th CRC
- fix some lag
- fix bug with mqtt timer
- fix crc remove on answer string
- added json string for inverter types
- added inverter type selection
- added configuration page
- added bootstrap based layout
- moved some html content to spiff
- mqtt raw data for QPIGS, QPIRI, QMOD, QID, QPIWS, QFLAG, QPI, QT, QET
- mqtt debug and serial debug enable in settings.h
- static device information from the QPIRI command
- PV power in Watt
- Inverter Operation Mode

**works with**
- pip / PCM  / MPI devices
- i solar 
- IGrid
- Many devices from EASUN
- and many many others based on the chinese solar inverter with a rj45 jack and usb port, primary identified by the display

# Known Bugs
- in Wifi Setup mode the inverter type must manualy input in uppercase Letters like "PCM" or "PIP"



![live](https://user-images.githubusercontent.com/44615614/139434909-d735c268-9cd7-4382-b100-d9d965da1500.jpg)
![edit](https://user-images.githubusercontent.com/44615614/139434941-79c6c013-d745-4eb1-adcc-4e0fd89dd4c3.jpg)


# Overview
Based around an ESP8266 WiFi microcontroller.

**PROGRAMMING:** You need to program it before hooking it up to the TTL board! Im using HW Serial since its so buggy using Software Serial and Wifi reliable so its not even worth it.


**UART0:** Talks to the inverter at 2400 baud and for programming. DISCONNECT RX before programming

**UART1:** Used for debugging only. Just connect another serial adaptor to D4 with 9600 baud.


**POWER:** Using a 3.3V DC Buck Converter that can handle up to 20V or a DC/DC or USB power currently.

**WIFI:** The system defaults to AP mode on first setup. Surf to "Solar-AP" AP and then surf to http://192.168.4.1/ On this page configure WIFI and MQTT.


# Parts required to build one

Most of the parts can be bought as modules, it's usually cheaper that way.

Approximate prices found on Ebay
- ESP8266 - Wemos D1 Mini ($2.66)
- MAX3232 module ($0.70) - it's just a max3232 with 5 capacitors on a tiny little board
- DC-DC buck module ($1.50) - 12-80v down to 5v
- LEDs ($0.50) - The leds are for status etc

Total cost: $5


Like my Work? Buy me a Coffee https://paypal.me/tobirocky
