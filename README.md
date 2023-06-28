# Features:
- WiFi Ap with setup
- All Data over MQTT, Writeable pints for ac Charge and Solar Charge
- Website with all essential data
- Web Update, Reset, Restart, Configure
- Building using Platform.io
- Live Data over Webpage

# Last Added:
- added configuration page
- added bootstrap based layout
- mqtt raw data for QPIGS, QPIRI, QMOD, QID, QPIWS, QFLAG, QPI, QT, QET
- static device information from the QPIRI command
- PV power in Watt
- Inverter Operation Mode

# ToDo: (Need Help)
- Restruct it to use with command lists of inverters
- create lists with commands for each protocol id and subprotocol to autoselect the right inverter

**works with**
- PIP devices
- i solar 
- IGrid
- Many devices from EASUN
- and many many others based on the chinese solar inverter with a rj45 jack and usb port, primary identified by the display
- Take a look at the device list in the wiki
- for P18 Protocol use this fork: https://github.com/BMBIT-oss/Solar2MQTT_P18



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

- ESP8266 - Wemos D1 Mini or ESP8266-01
- MAX3232 module Like this https://amzn.eu/d/8t3gk5t or https://bit.ly/3BFPqrw or with orginal cable https://www.amazon.de/dp/B09XWPTDYP
- DC-DC buck module - 12-80v down to 5v

# Completely assembled and tested PCB's

You are welcome to get fully stocked and tested PCB's. These are then already loaded with the lastest firmware.

![Solar-MQTT-PCB](https://user-images.githubusercontent.com/17761850/233859179-cc9c9075-b88a-4f38-b804-bc0f409cf8ce.png)

If interested see [here](https://all-solutions.store/?apply_coupon=NEWSHOP)

Until 15.07. there is also a discount of 5€ per PCB.

#
Questions?
[Join the Discord Channel (German / English)](https://discord.gg/zUQuTtcKuQ)
