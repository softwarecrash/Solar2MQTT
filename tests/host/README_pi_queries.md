# PI diagnostic inquiries

Run `node tests/host/run_pi_queries.cjs` from the repository root with Node.js,
a C++17 compiler and the project's ArduinoJson headers. `CXX` and
`ARDUINOJSON_INCLUDE` override the default paths.

The runner compiles the production polling loop, backup functions and command
queue against simulated UART and Modbus replies. It checks that known inquiries
wait for a polling boundary, share a slot with failed attempts and leave the
last measurement timestamp intact. It also covers a queued inquiry arriving
during a reply, unknown commands, invalid date arguments and the Modbus paths.
