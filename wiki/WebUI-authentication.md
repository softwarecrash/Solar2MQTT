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

- A configured username and password protect HTTP endpoints and new WebSocket connections on Wi-Fi, Ethernet and in AP mode.
- With no username configured, initial setup and the Web UI remain accessible without authentication.
- The fallback AP is open by default. Set `Access Point Password` in advanced network settings to secure it with an independent WPA2 password (8..63 characters).
- Saved passwords are not returned by normal settings endpoints. Blank password fields keep the existing value; `Clear password` removes it.
- The configuration backup contains passwords. Store it securely.

## Security note

This is local HTTP basic authentication only. Do not expose the Web UI directly to the internet.

If you need remote access, use:

- VPN
- reverse proxy with proper authentication and TLS
- other network-level protection
