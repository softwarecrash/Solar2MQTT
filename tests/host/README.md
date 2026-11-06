# Host regression tests

Run `node tests/host/run.cjs all` with Node.js 24, a C++17 compiler in `CXX`, and ArduinoJson 7.4.3 in `ARDUINOJSON_INCLUDE`. Assertions are always enabled.

Portable groups: `snapshot-memory`, `pi-transport`, `powmr-pv2`, `pv-total`, `qpigs2`, `pv2-polling`, `qpigs`, `harness`, `migration`, `zero-start`, `discovery-memory`.

Native ESP32 ABI groups: `pi`, `pi-energy`, `energy`, `modbus`, `queries`, `storage`, `battery-api`. On Linux use g++-multilib and `HOST_CXXFLAGS_JSON='["-m32"]'`; Windows unsigned long is already 32 bits. macOS CI runs the portable subset.

The regressions compile actual production functions. Host adapters replace only unavailable UART, clock, RTOS, HTTP and flash interfaces. Fault fixtures and injected failures are not hardware measurements.
