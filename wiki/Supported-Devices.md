This page lists inverter families and protocol variants that the ESP32 firmware supports.

Support here means that the code contains a dedicated implementation or a maintained fallback path. It does not guarantee that every vendor rebadge or firmware variant has already been tested on real hardware.

## Supported Devices

- `PI15`
- `PI16`
- `PI18`
- `PI30`
- `PI30_MAX`
- `PI30_REVO`
- `PI30_PIP_GK`
- `PI41`
- `PI30_UNKNOWN` fallback with raw capture
- `MODBUS_MUST`
- `MODBUS_DEYE`
- `MODBUS_ANENJI`
- `MODBUS_SMG`

## Notes

- Real-world confirmations are tracked separately on [Reported Working Devices](Reported-Working-Devices).
- If a device is only partly recognized but still answers, a debug report is often enough to improve support later.
