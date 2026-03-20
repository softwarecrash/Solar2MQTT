# Home Assistant integration

Solar2MQTT supports Home Assistant MQTT discovery. In normal use you do not need to add devices manually in Home Assistant.

## Requirements

1. Working MQTT broker
2. Solar2MQTT connected to Wi-Fi
3. MQTT configured in Solar2MQTT
4. Valid inverter or sensor data already visible in the Web UI

## Enable discovery

1. Open `MQTT`
2. Enable `Home Assistant discovery`
3. Save settings
4. Wait for MQTT reconnect

You can also trigger a rediscovery later from the `Debug` page with the `HA Discovery` button.

## What gets added automatically

- Solar inverter values
- Device information values
- DS18B20 temperature sensors

Only values that actually exist on your device are added.

## MQTT topics

Availability:

- `<MQTT topic>/Alive`

State topics:

- `<MQTT topic>/DeviceData/<name>`
- `<MQTT topic>/LiveData/<name>`
- `<MQTT topic>/DS18B20_<n>`

## JSON mode

JSON mode and Home Assistant discovery can be used together.

When both are enabled:

- the full JSON payload is still published
- the normal MQTT datapoint topics are still published so Home Assistant can create entities

## If entities do not appear

- verify MQTT is connected
- verify the Web UI already shows valid values
- check that the MQTT broker is receiving data from Solar2MQTT
- trigger `HA Discovery` once from the Debug page
