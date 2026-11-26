# GitHub OTA regression tests

Run the three commands from the repository root with Node.js, a C++17 compiler
and the project's ArduinoJson headers. Set `CXX` or `ARDUINOJSON_INCLUDE` to
override the defaults.

```sh
node tests/host/run_github_ota.cjs
node tests/host/test_ota_ui.cjs
node tests/host/writer/run.cjs
```

The updater uses the recorded GitHub release response for V2.0.4. The test
replays asset selection, version ordering, packet gaps, truncated responses,
HTTP failures, allocation errors and task creation failures. The ArduinoJson
parser and updater are compiled from production sources. GNU linker builds
also inject failed allocations.

The restart test compiles the production download and restart paths. It
checks that a successful download requests a restart, that failed or stalled
streams release the writer and that the main loop flushes persistent energy
and shuts down the inverter transport before rebooting. The storage and
restart interfaces use native test adapters.

The upload suite compiles the actual manual upload callbacks and checks
competing writers, incomplete or multiple files, disconnects and flash errors.
The UI suite evaluates the script from the actual firmware page.
