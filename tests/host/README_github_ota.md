# GitHub OTA regressions

`node tests/host/run.cjs ota ota-ui` compiles the complete production
`src/core/GitHubOtaUpdater.cpp`, with the real ArduinoJson parser. Only the
Arduino interfaces, HTTP/TLS transport, clock, RTOS tasks and flash writer are
local adapters. No updater or version-comparison implementation is copied into
the tests. `ota-ui` evaluates the scripts from the actual firmware page.

The tests never contact a device or GitHub, write real flash or install firmware.

## Recorded repository input

`fixtures/github-release-V2.0.4.json` is the unmodified UTF-8 response from:

https://api.github.com/repos/softwarecrash/Solar2MQTT/releases/latest

- Captured: 2026-11-05T20:40:12Z.
- Release: https://github.com/softwarecrash/Solar2MQTT/releases/tag/V2.0.4.
- Bytes: 14658.
- SHA256: `59a92ed30080692e83c71cc089a1538239c004a2db1cbf752c9539d4f8c4cef5`.

The four published board targets, asset names, URLs and release metadata come
from that response. The missing M5Stack asset is checked explicitly. Installed
version examples come from repository tags and `platformio.ini`. The original
comparator documents diagnostic suffixes; the prerelease ordering examples
follow https://semver.org/#spec-item-11 using the captured release base version.

Fault cases are changes to that same response: every byte truncation, removed
or changed fields, read interruption, HTTP/connection failure and repeated
ignored metadata. These are deliberate fault injections, not claimed readings
or firmware releases. No new inverter measurement data is introduced.

## Coverage

- Published assets, wrong board/version/file type, missing fields, fallback
  release name, draft/prerelease rejection and unchanged/newer installed builds.
- Streaming when a full-response String cannot be allocated; more than 500 KB
  of ignored metadata, without calling `HTTPClient::getString()`.
- All 14658 incomplete prefixes of the recorded response, invalid JSON, HTTP
  failures, closed/read-failing streams and invalid version identifiers.
- Development-prefix handling, stable/prerelease ordering, ignored build
  metadata, diagnostic suffix compatibility and numbers without integer overflow.
- Failed mutex/task creation, retries, duplicate/concurrent requests, a worker
  completing before its creator returns, and a pending-restart guard.
- GNU-linker builds on Windows/Linux inject failed real ArduinoJson allocations
  and growing reallocations. Shrinking reallocations are not faulted because
  ArduinoJson explicitly relies on those succeeding. macOS/Clang builds run
  the remaining cases without GNU linker allocation interception.
- The actual page script displays the concrete error, safely uses textContent,
  retries only `/ota/check`, and requires `update_available` for `/ota/update`.
- The separate `restart` group covers actual download/flush/restart production
  paths, including stalls, truncation, missing streams and timer wrap.

These tests do not reproduce TLS implementation details, ESP heap layout or
watchdog behavior. Hardware checks must confirm repeated HTTPS checks under
normal telemetry load, no unexpected reset, preserved counters and settings,
and the correct release/asset decision. A metadata check never proves that an
older published firmware is safe to install on a development build.

The release reader also replays packet gaps at 58 boundaries of the unmodified
release fixture, pauses at and immediately below the inactivity deadline, timeout,
retry and 32-bit clock wrap. This covers ESP32 TLS returning no bytes while the
connection is still open. It waits up to the existing 15 s inactivity limit;
it does not reconstruct a complete response String or retry firmware writes.

A failed metadata connection (negative HTTP transport code) is reconnected once
after releasing TLS resources and waiting one second. HTTP error statuses and
firmware writes are not retried. The same release fixture covers recovery and
permanent failure, with the retry count and delay explicitly bounded.
