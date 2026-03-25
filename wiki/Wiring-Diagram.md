# Wiring diagram

This page shows the default wiring for the available board targets.

The inverter UART pins, RS485 direction pin, DS18B20 pin and status LED pin can all be changed later in the Web UI under `Device Settings` if you want to use a different layout.

## wemos_d1_mini32:

This layout is meant as a drop-in replacement for the old D1 Mini style wiring.

| Function | GPIO | D1 Mini label |
|----------|------|---------------|
| Inverter RX to ESP32 | `GPIO19` | `D6` |
| Inverter TX from ESP32 | `GPIO23` | `D7` |
| RS485 DE / RE | `GPIO18` | `D5` |
| DS18B20 data | `GPIO21` | `D2` |
| Status LED | `GPIO2` | `D4` |

## ESP32-C3 SuperMini:

This target keeps the default wiring away from the USB pins.

| Function | GPIO |
|----------|------|
| Inverter RX to ESP32 | `GPIO20` |
| Inverter TX from ESP32 | `GPIO21` |
| RS485 DE / RE | `GPIO10` |
| DS18B20 data | `GPIO3` |
| Status LED | not used by default |

## ESP32-S3 SuperMini:

This target keeps the default wiring within the normal solder-pad pins and leaves the native USB pins free.

| Function | GPIO |
|----------|------|
| Inverter RX to ESP32 | `GPIO16` |
| Inverter TX from ESP32 | `GPIO15` |
| RS485 DE / RE | `GPIO7` |
| DS18B20 data | `GPIO6` |
| Status LED | not used by default |

## Waveshare ESP32-S3 ETH:

This target includes both inverter wiring and the default Ethernet module wiring.

### Inverter / sensor wiring

| Function | GPIO |
|----------|------|
| Inverter RX to ESP32 | `GPIO17` |
| Inverter TX from ESP32 | `GPIO18` |
| RS485 DE / RE | `GPIO15` |
| DS18B20 data | `GPIO21` |
| Status LED | not used by default |

### Ethernet module wiring

| Ethernet signal | GPIO |
|-----------------|------|
| W5500 MISO | `GPIO12` |
| W5500 MOSI | `GPIO11` |
| W5500 SCLK | `GPIO13` |
| W5500 CS | `GPIO14` |
| W5500 RST | `GPIO9` |
| W5500 INT | `GPIO10` |

## RS232

Use a `MAX3232` or equivalent level shifter between inverter serial and the ESP board.

- ESP TX -> level shifter RX -> inverter RX
- ESP RX <- level shifter TX <- inverter TX
- common ground is required

## RS485

Use an RS485 transceiver such as `MAX485`, `SN75176`, `SP3485` or equivalent.

- UART TX / RX use the selected board pins shown above
- DE / RE direction control uses the board-specific default pin shown above
- common ground is required

## Ethernet

For the `waveshare_esp32_s3_eth` target the firmware expects a `W5500` SPI Ethernet connection.

- use `3.3V`
- connect `GND`
- connect `MISO`, `MOSI`, `SCLK`, `CS`, `RST` and `INT`

## Notes

- `ESP32-C3` native USB uses `GPIO18` and `GPIO19`. The default `esp32c3_supermini` wiring leaves these free.
- `ESP32-S3` native USB uses `GPIO19` and `GPIO20`. The default `esp32s3_supermini` and `waveshare_esp32_s3_eth` wiring leaves these free.
- The current DS18B20 implementation supports up to `15` sensors on one shared 1-Wire bus.
- If you are unsure about the serial path, use the loopback test from the Debug page before connecting the inverter.
