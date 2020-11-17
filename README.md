# InverterOfThings Logger (FORK and total rebuild)
ESP8266 based WiFi interface for Voltronic Axpert MppSolar PIP inverters



# Overview
Based around an ESP8266 WiFi microcontroller.
Software is built using [Arduino for ESP8266](https://github.com/esp8266/Arduino). It's the simplest way to get the toolchain.

**PROGRAMMING:** You need to program it before hooking it up to the TTL board! Im using HW Serial since its so buggy using Software Serial and Wifi reliable so its not even worth it.

**UART0:** Talks to the inverter at 2400 baud and for programming. DISCONNECT RX before programming

**UART1:** Used for debugging only. Just connect another serie adaptor to TX.


**I2C:** Kept open for optional accessories

**GPIO:** Button on GPIO0, changes wifi mode, also used for programming when DTR pin isn't available.

**GPIO:** N/A

**POWER:** Inverter has +12V on pin 4 or 7 depending on the model. I used a cheapo DC-DC converter module to bring this down to ~5V, then a 1117-3.3V linear regulator to provide 3.3V. It draws about half a watt while the WiFi is connected.

**WIFI:** EDIT THIS... ITS NOT COMPLETE ---- The logger defaults to AP mode with the ssid 'Setup'. Connect to 192.168.4.1/wifi to setup the access point to connect to. You'll be given a list of APs in range to connect to. Select the AP, enter the password, then click submit to connect. The AP will be stored and the logger will connect automatically on next boot.

# Parts required to build one

Most of the parts can be bought as modules, it's usually cheaper that way.

Approximate prices found on aliexpress
- ESP-07 or ESP-13 module with breakout adapter board ($3.00)
- MAX3232 module ($0.50) - it's just a max3232 with 5 capacitors on a tiny little board
- DC-DC buck module ($1.00) - 12v down to 5v
- AMS1117-3.3V ($1.00) - Or any 3.3V regulator
- USB-TTL serial module ($1.50) - Make sure it has RTS and DTR outputs
- RJ45 socket with LEDs ($0.50) - The leds are for status etc.
- Tact switch ($0.00) - I'm sure you have an old button lying around.

Total cost: $7.50
