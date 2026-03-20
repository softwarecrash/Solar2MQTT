# Set parameters

Solar2MQTT can forward raw inverter commands. This is powerful, but dangerous.

**Warning:** commands are not syntax checked. Wrong commands can misconfigure the inverter.

## MQTT

Send commands to:

`<MQTT topic>/DeviceControl/Set_Command`

Read the answer from:

`<MQTT topic>/DeviceControl/Set_Command_answer`

The latest answer is also stored in:

`RawData/CommandAnswer`

## Web UI

Open `Debug` and use the `Command Sender` field.

## Useful commands

Typical PI serial examples:

- `QPI`
- `QMN`
- `QPIRI`
- `QPIGS`
- `QPIGS2`
- `QPIWS`
- `QALL`
- `autodetect`

## Notes

- Simulator commands are not stored. After reboot the device returns to normal hardware mode.
- Raw command answers are visible in the Debug page, MQTT answer topic and debug report.
- If you are unsure about a command, do not send it.
