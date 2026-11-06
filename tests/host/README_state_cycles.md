# Poll cycles, battery energy and restart regressions

See [README.md](README.md) for dependencies, the complete group list, CI and
the 32-bit timer ABI requirement. Run individual native groups from the
repository root:

```sh
node tests/host/run.cjs pi pi-energy
node tests/host/run.cjs energy storage battery-api
node tests/host/run.cjs queries modbus restart isolation
```

The original `run_state_cycles.cjs` entry point remains available. With no
arguments it runs `pi`, `pi-energy`, `energy`, `storage`, `battery-api` and
`queries`. Its `--modbus` and `--restart` arguments select those groups only.

## Production paths and adapters

Each group extracts only its required production functions. The `pi` group
uses the PI scheduler, status parsers, rollback, `SolarInverterService`
state updates, `SolarState.cpp` and flat/JSON MQTT publication. Its
`pi_startup_cases.h` replays the first-cycle/layout/rollback cases and producer
timing, independently of battery integration. It does not
load battery API/discovery functions or the actual custom-command queue.
Those paths have their own `battery-api` and `queries` groups. `pi-energy`
adds the real battery integration. `energy` and `storage` use the production
`EnergyBacklog.cpp` without a PI or MQTT implementation.

UART replies, clocks, MQTT transport, heap readings, LittleFS, Preferences and
partition APIs are substituted. The host semaphore uses a native mutex.
Production function bodies are unchanged. For older MinGW compilers only
the protocol-name table's inline linkage is adapted. Existing `switch(bool)`
and unused-variable/parameter warnings have narrow test-build exclusions.

## PI snapshots and timing

The `pi`, `pi-energy` and `queries` groups cover:

- Captured MAX/24 and GK/25 replies, documented PI41 L2 frames, and constructed
  REVO examples with and without supplemental QALL.
- API/MQTT reads during every UART wait and between poll steps; only complete
  raw/live inverter snapshots are published, using the established MQTT keys.
- Cold PI30/unknown detection, positive and zero PV values, 12/11/3-field
  transitions, CRC aborts, recovery and settings interrupting a poll.
- Late model detection after rejected QPIRI: conflicting standard/REVO cycles
  cannot reach the first published snapshot or renew battery freshness.
  Integer, decimal and night replies, transport recovery and absent initial
  rating information are exercised separately.
- Late MAX detection, a discarded cycle followed by QPIGS NAK, preservation
  of the preceding battery timestamps and late rating-CRC recovery.
- Rollback preserving independent temperature/local-energy updates, snapshot
  reset, and concurrent readers during a committed measurement update.
- An 11-second poll cadence, exact charge/discharge integrals at 500 W,
  stale measurements despite successful warning replies, serial loss,
  suspension, bounded timeouts and 32-bit timer wrap.

`queries` additionally compiles the actual custom-command constructor,
destructor, queue and inquiry classifier. Known read-only queries wait for
the first complete measurement, then consume at most one slot between full
normal cycles. Repeated 1.9-second queries, valid/NAK/no-answer/CRC replies
and timer wrap retain fresh battery samples and the expected integral.
Manual QPIGS/QPIGS2/QALL replies cannot replace a measurement or renew its
timestamp. Unknown/setting commands, a concurrent arrival, allocation failure
and native Modbus/PowMr command routing retain their separate checks.

## Integration, storage and API

`energy` compares integration with independently calculated 400 ms pulse
waveforms at four timer phases, irregular intervals, direction changes,
UART acceptance delays, expiry, malformed values, suspension and recovery.

`storage` covers legacy 32-byte/current 48-byte checkpoints, LittleFS/NVS
fallback, missing initialization metadata, corruption, sequence wrap and
repeated torn writes. Mount failures do not format a nonempty or unreadable
partition. Factory reset, failed cleanup, recovered filesystems, unavailable
storage and bounded checkpoint/retry intervals are checked separately.
Pending migration sessions survive reboot; baselines add once, with identical
retries, uncertain writes and device/boot guards exercised.

`battery-api` uses the actual migration/diagnostics/discovery functions and a
small inverter timing adapter, without reading the PI driver. It checks the
main-loop queue, validation, concurrent diagnostics and unchanged MQTT/HA
identities. Restore and explicit zero-start release the two reserved energy
configurations once, including reconnect, failed writes and repeated requests.
A positive session counted before zero-start is retained.

The command-line tool has separate local HTTP tests:

```sh
node tests/host/run.cjs migration zero-start
```

These exercise capture/prepare/restore and prepare-zero/start-zero, credentials
omission, identity and mode guards, exclusive output files, failed responses,
reboots, timeouts and retries with the same saved request. See
[battery-energy.md](../../Docs/battery-energy.md) for the device procedure.

## Modbus and restart

`modbus` compiles the production register decoder/cache and scheduler/timing
methods. Its setup adapter uses all seven production live-register tables;
callbacks extending unrelated nonbattery data and the separate PowMr PV2
probe are stubbed. The latter has the `powmr-pv2` group. Assertions cover:

- Poll intervals of 25, 100, 1000 and 5000 ms, including 26-second SMG and
  155-second MUST passes with independently calculated energy integrals.
- Complete successful acquisition of required battery inputs; status/static
  successes cannot renew incomplete, failed or invalid readings.
- Frozen expiry, recovery, valid zero/directional currents, cache acquisition
  time, delayed PowMr publication, diagnostic pause, disconnect and timer wrap.
- Profiles lacking complete canonical inputs cannot fabricate a valid battery
  reading. The pre-existing SMG negative Diematic rounding bias is recorded
  explicitly and is not changed by the timing correction.

`restart` compiles GitHub download/restart requests, the main-loop restart
handler and persistent-energy flush. HTTP/Update are local adapters with
fault-injected storage. Tests check success, download failures, exact deadline,
single consumption under the mutex, wrap, API restart, verified persistence
and reported checkpoint failure. No actual OTA or reboot occurs.

`isolation` builds groups from temporary partial source trees. It verifies
that unrelated PI, migration or query implementations can be absent, while a
missing required API function produces an explicit failure.

These checks do not establish physical PV-current measurement semantics or a
model-specific Q1 unit. They do not replace ESP32, RF-loss or physical flash
power-cut testing. Q1 scaling and QEX query-chain changes remain outside this
tool/test-infrastructure change.

The `pi` group additionally checks QMN priority and the legacy QMD model
fallback, including padded/invalid model fields, QMN CRC rollback, complete startup
and the original QPIRI ratings on MAX, GK, PI41 and REVO with/without QALL.
Only QMN rejection triggers QMD; timeout and CRC responses do not. The QMD
model is a firmware identifier, not an inferred marketing model name.

Permanent QMD CRC, NAK, no-answer and malformed replies cannot block startup
or trigger a new static pass before every measurement. Tests cover one attempt
per static acquisition, later recovery, retained valid model metadata and raw
error visibility. A failed optional QMD is not accepted as a valid model.

The `queries` group covers the additional exact legacy inquiry names
`QVFW4`, `QDI`, `QMCHGCR`, `QMUCHGCR`, `QMSCHGCR`, `QOPM`, `QCST`, `QCVT`
and `QFET`. Each runs between complete normal measurements, including accepted,
NAK, no-answer and CRC responses. These manual diagnostics do not create
sensors or replace normal measurements; settings and unknown commands keep
their existing handling. Failed initial static reads and repeated dynamic CRC
failures also release one diagnostic query per polling attempt; these queries
do not renew expired battery data or the dynamic-connection timestamp.
Recovery, GK/MAX/PI41/PI16/REVO with and
without QALL, timer wrap, and setting-command refresh remain covered.
