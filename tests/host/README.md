# Host regression tests

Run `node tests/host/run.cjs all` with Node.js 24, a C++17 compiler in `CXX`, and ArduinoJson 7.4.3 in `ARDUINOJSON_INCLUDE`. Assertions are always enabled.

Portable groups: `pi-transport`, `powmr-pv2`, `pv-total`, `qpigs2`, `pv2-polling`.

The existing production code is compiled by each regression. UART, clock, HTTP, RTOS or flash adapters replace the host-unavailable interfaces only where needed. Fault cases are deliberately modified fixtures, not additional claimed hardware measurements.

CI also builds wemos_d1_mini32, esp32c3_supermini, esp32s3_supermini, waveshare_esp32_s3_eth and m5stack_atom_lite.
