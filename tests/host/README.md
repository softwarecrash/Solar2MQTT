# Host regression tests

Run `node tests/host/run.cjs all` with Node.js 24, a C++17 compiler in `CXX`, and ArduinoJson 7.4.3 in `ARDUINOJSON_INCLUDE`. Assertions are always enabled.

Portable groups: `pi-transport`, `powmr-pv2`, `pv-total`, `qpigs2`, `pv2-polling`, `ha-discovery`, `status-led`.

The existing production code is compiled by each regression. UART, clock, HTTP, RTOS or flash adapters replace the host-unavailable interfaces only where needed. Fault cases are deliberately modified fixtures, not additional claimed hardware measurements.

CI also builds wemos_d1_mini32, esp32c3_supermini, esp32s3_supermini, waveshare_esp32_s3_eth and m5stack_atom_lite.

## Home Assistant discovery switch

`node tests/host/run_ha_discovery.cjs` replays the production MQTT reconnect,
polling, discovery dispatch and cleanup callback paths with a recording transport.
It checks that disabled discovery sends neither configurations nor empty retained
HA messages, including manual requests, queued work, reconnects and in-flight
cleanup replies. Enabled discovery must still publish and clean up normally.
This suite is also included in `node tests/host/run.cjs all` and `portable`.

## Status LED backends

`node tests/host/run_status_led.cjs` compiles the production status LED service
with PWM and addressable backends. It checks pulse mapping, brightness, disabling,
pin changes, RMT initialization failure and release, frame deduplication, and a
five-second simulated pulse cycle. GPIO/RMT functions and task timing are mocked;
frame counts are scheduling checks, not hardware CPU or heap measurements.
