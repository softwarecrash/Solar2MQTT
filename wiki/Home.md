Welcome to the Solar2MQTT wiki.

This wiki keeps the familiar page layout from the older project, but the content is updated for the current version.

## Current scope

- Build targets: `wemos_d1_mini32`, `wemos_d1_mini32_dbg`
- Default pin layout matches the familiar Wemos D1 Mini labeling
- Supports many common PI and Modbus inverter families
- Web UI with status page, Wi-Fi setup, MQTT settings, device settings, firmware update, debug tools and Web Serial
- MQTT flat topics or JSON mode
- Home Assistant discovery
- DS18B20 temperature sensor support
- GitHub OTA update, local OTA upload, settings backup and restore
- Optional simulator mode for testing without a real inverter

## Quick links

- [Wiring Diagram](Wiring-Diagram)
- [Wiring temperature sensors](Wiring-temperature-sensors)
- [Datapoints and units](Datapoints-and-units)
- [Set parameters](Set-parameters)
- [WebSerial](WebSerial)
- [Home Assistant integration](HomeAssistant-integration)
- [WebUI authentication](WebUI-authentication)
- [Reset](Reset)
- [Blink Codes](Blink-Codes)
- [Confirmed Working Device List](Confirmed-Working-Device-List)
- [Get Debug Log from WatchPower](get-Debug-Log-from-watchpower)

## Notes

- Available datapoints depend on the inverter family and the replies the device actually sends.
- If your inverter is not recognized properly, the `Debug` page is the best place to collect useful information.
- The status page stays compact on purpose. Detailed information is on the `Debug` page.
