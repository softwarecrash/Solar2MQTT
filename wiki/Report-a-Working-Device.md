If your inverter works with the ESP32 version, please report it so it can appear in the public compatibility list.

## How to report

1. Open the Solar2MQTT Web UI.
2. Go to `Device Settings`.
3. Click `Report Device`.
4. Fill in the missing manufacturer, model and transport fields.
5. Submit the form.

If you cannot use the ESP Web UI, you can use the manual web form:

- https://solar2mqtt-reportdevice.softwarecrash.de/?manual=1

If neither option works, there is also a GitHub issue form for manual working-device reports.

## What is sent

- manufacturer
- model name as written on your device
- transport type (`RS232`, `TTL`, `RS485`)
- detected protocol
- detected protocol ID
- detected device model
- device firmware version if available
- ESP build variant
- optional note

## Notes

- No GitHub account is required.
- Reported entries are normalized automatically before they appear in the public list.
- The current list is available on [Reported Working Devices](Reported-Working-Devices).
