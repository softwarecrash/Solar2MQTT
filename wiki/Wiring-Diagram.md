# Wiring diagram

This page shows the normal default wiring.

## Default build: `wemos_d1_mini32`

The project uses ESP32 hardware UART with the D1 Mini replacement layout:

| Function | GPIO | D1 Mini label |
|----------|------|---------------|
| Inverter RX to ESP32 | `GPIO19` | `D6` |
| Inverter TX from ESP32 | `GPIO23` | `D7` |
| RS485 DE / RE | `GPIO18` | `D5` |
| DS18B20 data | `GPIO21` | `D2` |
| Status LED | `GPIO2` | `D4` |

These are the default pins. The inverter UART pins can also be changed later in the Web UI device settings.

## RS232

Use a `MAX3232` or equivalent level shifter between inverter serial and the ESP32.

- ESP32 TX -> level shifter RX -> inverter RX
- ESP32 RX <- level shifter TX <- inverter TX
- common ground is required

## RS485

Use an RS485 transceiver such as `MAX485`, `SN75176`, `SP3485` or equivalent.

- UART TX / RX use the pins above
- DE / RE direction control uses `GPIO18 / D5` by default
- common ground is required

## Notes

- The default mapping is chosen so a Wemos D1 Mini ESP32 can replace a Wemos D1 Mini ESP8266 with the same board-side pin labels.
- USB serial logging stays on the normal ESP32 USB / UART path and is separate from the inverter UART.
- If you are unsure about the serial path, use the loopback test from the Debug page before connecting the inverter.
