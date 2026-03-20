# Get debug data from WatchPower / SolarPower

If your inverter is not recognized correctly, missing fields, or answering with unexpected raw strings, vendor PC software logs are still useful.

## First use the built-in ESP32 project tools

Before collecting vendor logs, use the built-in debug paths:

1. Web UI -> `Debug` -> `Preview JSON`
2. Web UI -> `Debug` -> `Preview Report`
3. Web UI -> `Debug` -> `Download Report`
4. Web UI -> `Web Serial`
5. MQTT raw topics under `RAW/*`
6. the live data shown by the Web UI

These usually already contain enough material for protocol work.

## Why this is useful

The current project keeps raw answers even for not-yet-supported but responding devices.

That means the following are especially valuable:

- `QPI`
- `QMN`
- `QPIRI`
- `QPIGS`
- `QPIGS2`
- `QPIWS`
- `QALL`
- `Q1`
- `CommandAnswer`

## WatchPower / SolarPower logs

- install the vendor PC software
- connect the inverter and let it poll for a while
- collect the raw protocol files or debug files
- zip the full log folder
- include device model, protocol guess and transport type

## What helps most in a bug report

- debug report from Solar2MQTT
- raw MQTT topics or a JSON preview from the Debug page
- vendor software log files
- photo of the inverter label
- note whether the issue is wrong detection, missing values or unstable values
