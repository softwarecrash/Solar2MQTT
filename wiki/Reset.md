# Reset

There are two ways to erase the stored settings.

## Over the Web UI

- Open `Debug`
- Click `Factory Reset`
- Confirm the reset

This clears the saved settings and reboots the device.

## Emergency reset by repeated restart

There is also an emergency reset option if the Web UI is no longer reachable:

- restart or power-cycle the device repeatedly
- after `6` counted resets, the device erases settings and reboots
- the counter is cleared automatically if a boot stays up for about `10` seconds

This is intended as a recovery path when the Web UI is no longer reachable.

## What is erased

- Wi-Fi settings
- MQTT settings
- Web UI credentials
- device UART pin settings
- other saved preferences
