# Wiring temperature sensors

Solar2MQTT supports DS18B20 temperature sensors.

## Default pin mapping by build target

| Build target | DS18B20 data | Note |
|--------------|--------------|------|
| `wemos_d1_mini32` | `GPIO21` | `D2` on the D1 Mini style layout |
| `esp32c3_supermini` | `GPIO3` | |
| `esp32s3_supermini` | `GPIO6` | |
| `waveshare_esp32_s3_eth` | `GPIO21` | |

The DS18B20 pin can also be changed later in the Web UI under `Device Settings`.

Set the pin to `-1` if you want to disable the DS18B20 bus completely.

## Standard one-wire wiring

- DS18B20 `VCC` -> `3.3V`
- DS18B20 `GND` -> `GND`
- DS18B20 `DATA` -> selected DS18B20 data pin
- add a `4.7k` pull-up resistor between `DATA` and `3.3V`

Multiple DS18B20 sensors can share the same wire.

## Maximum number of sensors

The current firmware supports up to `15` DS18B20 sensors on one 1-Wire bus.

In practice the reliable number also depends on wiring quality, cable length, pull-up resistor and power supply.

## Sensor numbering

Sensors are published as:

- `DS18B20_1`
- `DS18B20_2`
- `DS18B20_3`
- ...

The numbering is usually stable as long as the same sensors stay connected.

## Where the values appear

- MQTT topic `<baseTopic>/DS18B20_<n>`
- MQTT topic `<baseTopic>/EspData/DS18B20_<n>`
- Home Assistant discovery if enabled

## Notes

- If a sensor is removed, its value disappears again after a short time.
- If no sensors are found, no temperature values are shown.
