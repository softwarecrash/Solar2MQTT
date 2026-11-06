# QPIGS2 regression test

Run from the repository root after installing the project dependencies:

```sh
node tests/host/run_qpigs2.cjs
node tests/host/test_pv_total.cjs
```

The C++ test compiles the production `QPIGS2.h`, parsing helpers, protocol types
and ArduinoJson with substitutes for UART and Arduino String. It needs a native
C++17 compiler (`CXX`, default `c++`) and ArduinoJson (`ARDUINOJSON_INCLUDE`,
default `.pio/libdeps/wemos_d1_mini32/ArduinoJson/src`). Temporary builds are
removed on success and failure.

Coverage: recorded MAX three-field reply from a MAX device, two recorded
twelve-field replies from a legacy PIP device, existing PI41 L2 layout, unchanged
PV1 names/values, night zero, zero voltage, malformed/truncated/oversized legacy
replies, CRC/NAK/no-answer handling, stale-field clearing, recovery and dispatch.

The twelve-field layout accepts unsigned integers or plain decimal numbers with
varying precision, including integer zero and leading zeros in all numeric
fields. It requires digits before and after an optional single decimal point.
The whole-number part is limited to 99999 (the widest documented numeric field);
fractional digits remain bounded by the existing 127-byte payload limit.
The field count stays exactly twelve and field 4 stays exactly eight binary bits.
Signs, exponents, NaN/Inf, decimal commas and trailing text are rejected.
The existing production conversion still rounds stored numbers to two decimals.

Additional checks: 33 equivalent field representations; mixed precisions;
unused SCC3 fields; integer zero; division by zero; five-digit numeric bounds;
187 malformed numeric/empty-field cases; invalid status lengths/content;
127/128-byte payload boundary; stale-field clearing and recovery. The three-
and eleven-field branches are unchanged and are replayed with extra precision.
All three layouts also exercise NAK/NOA/CRC, empty/malformed/oversized replies,
clearing of owned fields and recovery. A CRC reply preserves the previous values
until the polling cycle rolls back; owned fields are no longer cleared while
waiting for the reply. Numeric-token/status-bit validation is
extended only for the twelve-field layout; the older branches are not rewritten.

The original short MAX and PI41 calculations remain unchanged. In particular,
`01.9 388.9 00745` still produces 739 W input power from voltage * current,
alongside 745 W charging power. The twelve-field layout copies its reported PV2
charging watts to the existing `PV2_Charging_Power` and `PV2_Input_Power` fields.
It derives `PV2_Input_Current` as those watts / PV voltage, with the existing
zero-voltage guard and two-decimal rounding. This is a calculated current, not
an independent panel-current measurement.

Section 2.10 of `Protocol/PI30_Communication-Protocol-20150924-Customer.pdf`
labels the first raw field as PV input current 2. That field is not used in
this branch. Its physical interpretation on the captured device has not been
independently established; do not assert a general battery-side meaning.
Likewise, the protocol calls the final field total PV charging power; the
existing `PV_Total_Input_Power` descriptor is retained without a name change.

The captured twelve-field device was identified from its label as an MPP Solar
PIP 4048MSD (5000 VA / 4000 W; main firmware 75.20, SCC replies 1.02). During
the 2026-11-05 display comparison, PV1 was 44 W in both sources; PV2/total were
28/72 W on the LCD and 27/71 W in the serial reply, API and fresh MQTT data.
Both PV voltages were approximately 82 V. This documents the observed device;
it neither justifies a one-watt offset nor establishes current semantics for
every device with twelve fields. The LCD's aggregate PV-to-battery 1 A reading
is not an independent PV2 input-current reference.

Existing descriptors are used; QEX, Q1, discovery, simulation and UI behavior
are outside this fix. See `Protocol/PROTOCOL_REFERENCE.md` for all field positions.

`--compat` runs the existing-format cases and prints complete parsed snapshots.
This allows a direct comparison against the original parser. The full test
must fail with the original parser because that parser ignores the twelve-field
reply. Host tests replay captured data; they do not flash either board or prove
live UART/MQTT behavior.

`run_state_cycles.cjs` additionally tests complete polling cycles, concurrent
snapshot readers, actual MQTT publication functions and battery-data freshness;
see `README_state_cycles.md`.

`--recorded` also prints the two captured twelve-field daytime replies and a
night reply. Compare it before/after a formatting-only change. `--formats`
runs the flexible-number and invalid-token cases; it must fail with the strict
twelve-field validator because numeric-equivalent representations were rejected.
