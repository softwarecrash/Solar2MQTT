# Device blink codes

Every 5 seconds:

- `1x` everything is OK
- `2x` no inverter connection
- `3x` no MQTT connection
- `4x` no Wi-Fi connection

## Default LED pin

The default status LED pin is:

- `GPIO2` (`D4` on the Wemos D1 Mini ESP32 layout)

The LED pin can also be changed in `Device Settings`.

## Brightness

The default LED brightness is `128`.

Brightness can be adjusted in `Device Settings`.
