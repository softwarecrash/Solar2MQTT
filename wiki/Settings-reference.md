# Settings reference

This page explains the settings available in the Web UI.

## General behavior

- All settings are stored in NVS and survive a reboot.
- Numeric values are clamped to the supported range when they are saved or restored from backup.
- Saving network settings can reconfigure the current connection immediately.
- `Poll Interval [ms]` and `Send interval [s]` are different settings:
  - `Poll Interval [ms]` controls inverter communication over UART / RS232 / RS485.
  - `Send interval [s]` controls how often MQTT publishes the current state.

## Network settings

| Setting | What it does | Default / range | Notes |
| --- | --- | --- | --- |
| Prefer Ethernet when available | Uses Ethernet first when a supported LAN board has link. | Default: `on` for LAN builds, hidden on Wi-Fi-only builds | Wi-Fi stays available as fallback. |
| Device Name | Friendly device name shown in the UI and used for MQTT discovery metadata. | Default: `Solar2MQTT` | Also becomes the mDNS host name, normalized to lowercase letters, digits and dashes, for example `solar2mqtt.local`. |
| SSID | Primary Wi-Fi network. | Default: empty | If no SSID is configured, the device cannot join your normal Wi-Fi. |
| Password | Password for the primary Wi-Fi network. | Default: empty | Required for secured networks. |
| Lock to selected BSSID | Locks Wi-Fi connection to the selected access point MAC address. | Default: `off` | Useful for mesh or roaming environments. It only has an effect when a valid BSSID was stored from the scan list. |
| Fallback SSID | Secondary Wi-Fi network used if the primary one is unavailable. | Default: empty | Optional. |
| Fallback password | Password for the fallback SSID. | Default: empty | Optional. |
| Static IP | Static IP address for Wi-Fi or Ethernet. | Default: empty | Static addressing is only used when `Static IP`, `Subnet Mask`, `Gateway` and `DNS Server` are all filled with valid values. Otherwise DHCP is used. |
| Subnet Mask | Subnet mask for static addressing. | Default: empty | Must be valid together with the other static fields. |
| Gateway | Gateway for static addressing. | Default: empty | Must be valid together with the other static fields. |
| DNS Server | DNS server for static addressing. | Default: empty | Must be valid together with the other static fields. |
| Username | Enables HTTP basic authentication for the Web UI. | Default: empty | If the username is empty, Web UI authentication is disabled. |
| Password | Password for Web UI authentication. | Default: empty | In AP mode authentication is skipped so initial setup stays reachable. See [WebUI authentication](WebUI-authentication). |

## MQTT settings

| Setting | What it does | Default / range | Notes |
| --- | --- | --- | --- |
| MQTT Host | Broker host name or IP address. | Default: empty | MQTT is disabled when this field is empty. |
| Port | Broker port. | Default: `1883`, range `1..65535` | Typical TLS port is `8883`. |
| User | MQTT username. | Default: empty | Optional. |
| Password | MQTT password. | Default: empty | Optional. |
| Base topic | Root topic used for all publishes and subscriptions. | Default: `Solar` | Example: `Solar/LiveData/...` |
| Trigger topic | Topic that triggers an immediate full MQTT publish when a message arrives. | Default: empty | Payload content is ignored. Useful for external polling or automations. |
| Send interval [s] | Periodic MQTT publish interval. | Default: `30`, range `0..86400` | `0` disables the periodic timer. Reconnects, commands, DS18B20 updates and trigger-topic messages can still publish data immediately. |
| Use SSL/TLS | Uses TLS for the MQTT connection. | Default: `off` | Current firmware encrypts the connection but does not validate the broker certificate. |
| Enable JSON mode | Publishes grouped JSON data under `<base topic>/Data`. | Default: `off` | Intended as an alternative to only using many single-value topics. JSON mode cannot be enabled together with Home Assistant discovery. |
| Enable Home Assistant discovery | Publishes Home Assistant discovery topics. | Default: `off` | Enabling this switches JSON mode off automatically. See [Home Assistant integration](HomeAssistant-integration). |

## Device settings

| Setting | What it does | Default / range | Notes |
| --- | --- | --- | --- |
| UART RX GPIO | ESP32 RX pin connected to the inverter interface. | Default depends on build target, range `-1..48` | Wrong values will break inverter communication. |
| UART TX GPIO | ESP32 TX pin connected to the inverter interface. | Default depends on build target, range `-1..48` | Wrong values will break inverter communication. |
| RS485 DIR GPIO | Direction pin for RS485 transceivers. | Default depends on build target, range `-1..48` | Use `-1` if your hardware does not need a direction pin. |
| Poll Interval [ms] | Delay between individual inverter requests on the serial bus. | Default: `100`, range `25..5000` | Lower values mean faster updates but more UART / RS485 traffic and more load on sensitive devices. Higher values reduce bus load but data updates become slower. This is not the same as the full refresh time of all values because one complete PI polling cycle needs multiple requests. |
| DS18B20 GPIO | OneWire pin used for external DS18B20 sensors. | Default depends on build target, range `-1..48` | Use `-1` to disable. Multiple DS18B20 sensors can share the same pin and are published as `DS18B20_1`, `DS18B20_2`, and so on. |
| Status LED GPIO | GPIO used for the status LED output. | Default depends on build target, range `-1..48` | Use `-1` to disable the status LED. Blink meanings are documented on [Blink Codes](Blink-Codes). |
| Status LED Brightness | PWM brightness for the status LED. | Default: `128`, range `0..255` | `0` keeps the LED dark even if a pin is configured. |

## Default GPIOs by build target

| Build target | Status LED | UART RX | UART TX | RS485 DIR | DS18B20 | Ethernet |
| --- | --- | --- | --- | --- | --- | --- |
| `wemos_d1_mini32` | `2` | `19` | `23` | `18` | `21` | no |
| `wemos_d1_mini32_dbg` | `2` | `19` | `23` | `18` | `21` | no |
| `esp32c3_supermini` | `-1` | `20` | `21` | `10` | `3` | no |
| `esp32s3_supermini` | `-1` | `16` | `15` | `7` | `6` | no |
| `waveshare_esp32_s3_eth` | `-1` | `17` | `18` | `15` | `21` | yes |

If you change GPIO settings and lose communication, use the [Reset](Reset) options or restore a known-good configuration backup.
