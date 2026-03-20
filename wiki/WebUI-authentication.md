# WebUI authentication

Solar2MQTT supports optional HTTP basic authentication for the Web UI.

## Enable it

1. Open `Wi-Fi`
2. Expand `Advanced Network Settings`
3. Fill `Username`
4. Fill `Password`
5. Save settings

## Disable it

1. Open `Wi-Fi`
2. Expand `Advanced Network Settings`
3. Clear the username
4. Save settings

## Current behavior

- Authentication is skipped in AP mode so initial setup remains reachable.
- Once the device is on your normal Wi-Fi, the configured username and password protect the Web UI.

## Security note

This is local HTTP basic authentication only. Do not expose the Web UI directly to the internet.

If you need remote access, use:

- VPN
- reverse proxy with proper authentication and TLS
- other network-level protection
