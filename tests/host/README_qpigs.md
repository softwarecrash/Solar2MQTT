# QPIGS layout regression tests

Run from the repository root with Node.js, a native C++17 compiler and the
project's ArduinoJson headers.

```sh
node tests/host/run_qpigs.cjs
```

Set `CXX` to select a compiler and `ARDUINOJSON_INCLUDE` to override the default
`.pio/libdeps/wemos_d1_mini32/ArduinoJson/src` include directory.

The test builds the production QPIGS parser, field helpers and protocol
refinement. A native test substitute supplies the inverter replies. Cases cover

- Captured MAX and PIP-GK day and night replies, including integer and decimal
  current fields. A simulated 7 A discharge stays in amperes.
- PI16, PI41 and identified PI30 variants with conflicting layout hints.
- Explicit and inferred REVO with and without QALL, including the original
  voltage and binary checks for the no-QALL fallback. REVO frames use the
  documented field values and widths in the V03 protocol.
- The first QPIGS before QPIWS refinement, later QALL recovery, and NAK,
  no-answer, CRC and incomplete responses.

`--startup` runs the first detection cycle alone. `--compat` prints snapshots
for comparison with the original parser. The full test reproduces the
PIP-GK regression against the original parser. The cases replay parser inputs
on the host.
