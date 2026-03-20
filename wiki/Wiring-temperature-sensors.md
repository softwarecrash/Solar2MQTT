# Wiring temperature sensors

Solar2MQTT supports DS18B20 temperature sensors.

## Default pin mapping

| Function | GPIO | D1 Mini label |
|----------|------|---------------|
| DS18B20 data | `GPIO21` | `D2` |

## Standard one-wire wiring

- DS18B20 `VCC` -> `3.3V`
- DS18B20 `GND` -> `GND`
- DS18B20 `DATA` -> `GPIO21 / D2`
- add a `4.7k` pull-up resistor between `DATA` and `3.3V`

Multiple DS18B20 sensors can share the same wire.

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
