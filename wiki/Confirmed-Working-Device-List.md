This page keeps the old wiki slot, but for the ESP32 version it should mainly track devices that were actually tried by users.

## Current note

The supported device list is not complete yet. Some inverter families already work, but not every model has been re-confirmed by users.

## Supported inverter families

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

## What helps when reporting a working device

Please report:

- inverter brand and exact model
- transport type (`RS232`, `TTL`, `RS485`)
- detected protocol name shown in the Web UI if available
- whether live values, MQTT and Home Assistant discovery work correctly
- a debug report if something is missing

## Important note

If a device is only partly recognized but still answers, a debug report is often enough to improve support later.
