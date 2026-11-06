# QPIGS layout regression tests

Run from the repository root after installing the project dependencies:

```sh
node tests/host/run_qpigs.cjs
node tests/host/run_qpigs2.cjs
```

Requirements: native C++17 compiler (`CXX`, default `c++`) and ArduinoJson
(`ARDUINOJSON_INCLUDE`, default `.pio/libdeps/wemos_d1_mini32/ArduinoJson/src`).
The runner builds the complete production QPIGS/QPIGS2 parsers, production
field helpers, protocol predicates and ArduinoJson. Only String and UART are
substituted, along with logging. The runner also compiles the complete production
`refineProtocol`, its response/signature helpers and mode mapper. Fixed-protocol
cases use the production forced-protocol guard; startup cases enable actual
refinement. Temporary builds are removed.

Coverage:

- Captured PIP-GK replies with integer PV current, positive PV and night zero.
- Synthetic 7 A discharge and integer/decimal formatting: discharge stays in
  amperes and never becomes a PV daily-energy reading.
- PIP-GK/MAX/PI41/PI16 take precedence over conflicting layout hints.
- Generic PI30/PI30_UNKNOWN retain the original no-QALL REVO heuristic, including
  its integer/voltage/binary checks. A 25+ field QPIRI identifies a standard
  layout and blocks that ambiguous heuristic even before QPIWS has refined the
  first PI30 cycle. Explicitly recognized REVO remains supported independently.
- Explicit REVO still works when QALL is rejected or unavailable. REVO fixture
  values/layout come from protocol V03 sections 1.2/1.4; no physical REVO board
  was available. Existing REVO calculations and units are outside this change.
- NAK/no-answer/CRC handling, incomplete signatures, layout transitions and
  recovery, existing topic names, combined QPIGS + QPIGS2 for both installations.
- First dynamic cycle from PI30 and PI30_UNKNOWN: no QMOD/QPIWS yet, correct GK
  values on the first QPIGS, MAX rating refinement, REVO without QALL, later
  QPIWS refinement and QALL becoming available/unavailable. `--startup` runs
  this group alone. Startup sequencing is replayed; the tests do not execute
  the UART/Modbus probing or timed `loop()` scheduler and do not prove hardware
  autodetection for every model. Unknown devices without a conclusive rating
  still use the original heuristic and retain its inherent ambiguity.

`--compat` prints complete snapshots of existing MAX, PI16, PI41, 17-field PI30
and explicit/inferred/no-QALL-fallback REVO cases for comparison with the unchanged parser.
The full test must fail with the original parser on the captured GK regression.
Host replay does not prove live UART/MQTT operation. These tests never flash
a board or change Home Assistant, MQTT discovery, connection timing or QEX.
