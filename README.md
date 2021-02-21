# Known Bugs:
- first AP mode timeout too fast
- SSID with spaces not working for setup mode

# InverterOfThings Logger (FORK and total rebuild)
ESP8266 based WiFi interface for Voltronic Axpert MppSolar PIP inverters


**Note2 that all below info is from the forked project and in short this will be updated when all is refactored and perhaps working....**

# Overview
Based around an ESP8266 WiFi microcontroller.
Software is built using [Arduino for ESP8266](https://github.com/esp8266/Arduino). It's the simplest way to get the toolchain.

**PROGRAMMING:** You need to program it before hooking it up to the TTL board! Im using HW Serial since its so buggy using Software Serial and Wifi reliable so its not even worth it.


**UART0:** Talks to the inverter at 2400 baud and for programming. DISCONNECT RX before programming

**UART1:** Used for debugging only. Just connect another serie adaptor to TX.


**GPIO:** NOT IN USE Button on GPIO0, changes wifi mode, also used for programming when DTR pin isn't available.


**POWER:** Using a DC/DC or USB power currently. My gear dont have any power on the serial port

**WIFI:** The system defaults to AP mode on first setup. Surf to "SetSol" AP and then surf to http://192.168.4.1/ On this page configure WIFI and MQTT. Then you need to reboot the device either by the reboot botton or hard reboot. No changes are applied properly before reboot


# Parts required to build one

Most of the parts can be bought as modules, it's usually cheaper that way.

Approximate prices found on Ebay
- ESP8266 - Wemos D1 Mini - https://ebay.to/3660Nsy - ($2.66)
- MAX3232 module ($0.70) - it's just a max3232 with 5 capacitors on a tiny little board  - https://ebay.to/3la4G45
- DC-DC buck module ($1.50) - 12-80v down to 5v https://ebay.to/3lbf3V7
- LEDs ($0.50) - The leds are for status etc

Total cost: $5

Links above are affiliated and give me a tiny commission if used. 
