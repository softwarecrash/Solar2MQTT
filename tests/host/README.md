# Host regressions

Run from the repository root with Node.js 24, a C++17 GCC/Clang compiler and
ArduinoJson 7.4.3. These tests use local adapters and loopback HTTP; they never
connect to an inverter, Home Assistant or a real MQTT broker.

```sh
node tests/host/run.cjs --list
node tests/host/run.cjs all
node tests/host/run.cjs qpigs2 zero-start
```

Set `CXX` to the compiler executable. Set `ARDUINOJSON_INCLUDE` to an
ArduinoJson `src` directory, or first install the firmware dependencies under
`.pio/libdeps/wemos_d1_mini32/ArduinoJson/src`. The tests need no other
PlatformIO build artifacts. For example, a clean checkout can use:

```sh
git clone --depth 1 --branch v7.4.3 https://github.com/bblanchon/ArduinoJson.git .test-deps/ArduinoJson
export ARDUINOJSON_INCLUDE="$PWD/.test-deps/ArduinoJson/src"
export CXX=g++
```

On PowerShell, set environment variables using `$env:CXX` and
`$env:ARDUINOJSON_INCLUDE` instead of `export`. MinGW is supported on Windows.

## Test groups

| Group | Production code or contract checked |
| --- | --- |
| `pi-transport` | Serial reply validation, timeouts and transport errors |
| `powmr-pv2` | Separate PowMr Modbus PV2 probe |
| `pv-total` | Existing web PV power helpers |
| `qpigs` | PI QPIGS layouts, REVO detection and first-cycle cases |
| `qpigs2` | Short, legacy 12-field and PI41 L2 PV2 frames |
| `pv2-polling` | Repeated QPIGS/QPIGS2 polling replay for GK, MAX and PI41 PV2 values |
| `migration` | Existing capture/prepare/restore command-line HTTP contract |
| `zero-start` | Explicit zero-baseline preparation/apply and retry contract |
| `harness` | Production function extraction and assertion-safe compiler flags |
| `ota` | Complete GitHub updater: recorded release, streaming, version ordering and task lifecycle |
| `ota-ui` | Actual firmware page error/retry/update behavior |
| `status-memory` | Actual status/data serialization, allocation failures, document lifetimes and zero-client WebSocket behavior |
| `snapshot-memory` | Atomic producer/local snapshots, allocation failures, source recovery and failed reader copies |
| `snapshot-serialization` | Exact API JSON without a reader document clone, overlays, write failures and concurrent commits |
| `discovery-memory` | Actual HA helpers, allocation-free incremental topic collection and unchanged forced reconciliation |
| `firmware-writer` | Actual OTA/upload callbacks, exclusive flash ownership, failures and disconnect cleanup |
| `pi` | PI scheduler, complete snapshots and MQTT publication |
| `pi-energy` | PI battery acquisition times, freshness, startup and integration |
| `energy` | Battery integration against independently calculated waveforms |
| `storage` | Checkpoints, LittleFS/NVS, recovery, migration and write bounds |
| `battery-api` | Migration queue, diagnostics and existing discovery identities |
| `queries` | Actual read-only query queue between completed measurement cycles |
| `modbus` | Production decoder/cache and battery timing for all seven profiles |
| `restart` | GitHub OTA and API restart scheduling with persistent-energy flush |
| `isolation` | Groups still build without unrelated implementations; missing required functions fail |

`all` runs all 25 groups. `portable` runs the sixteen portable groups. See
[README_state_cycles.md](README_state_cycles.md) for the adapters and runtime
cases covered by the native groups. Original individual runners remain usable.

## ESP32 timer representation

The eight native groups and `isolation` require a 32-bit `unsigned long`, as on
ESP32. A compile-time check prevents a 64-bit host ABI from silently hiding
timer-wrap failures. Windows MinGW already has a 32-bit `unsigned long`.
On 64-bit Linux, install `g++-multilib` and pass:

```sh
export HOST_CXXFLAGS_JSON='["-m32"]'
node tests/host/run.cjs all
```

`HOST_CXXFLAGS_JSON` is a JSON array of individual compiler arguments, not a
shell command. Assertions remain enabled even if the surrounding environment
uses release-build flags. The runner rejects `-DNDEBUG` explicitly. Native
builds use warnings as errors, with narrow exclusions for existing mock and
production warnings documented in the runner.

Current macOS compilers do not support these 32-bit native builds. Run
`node tests/host/run.cjs portable` there; that result does not claim coverage
of the timed native groups.

## CI and independently reviewable changes

[host-tests.yml](../../.github/workflows/host-tests.yml) runs all groups on
Windows and Linux, portable groups on macOS, and firmware builds for the five
configured board targets. It downloads the pinned ArduinoJson version without
requiring a prior firmware build. A local workflow syntax check is separate
from an actual GitHub Actions run.

The group generators load only the production methods required by that
group. The isolation test removes unrelated PI, battery-API or query methods
in temporary copies and recompiles the relevant group. Required methods are
never skipped when missing. Sources are copied verbatim into the native
harness; hardware transport, clocks and persistent storage are adapted.

When splitting the firmware changes into commits or pull requests, include
the corresponding tests and shared support files in each applicable change.
Run that intermediate checkout's own groups and firmware build. Passing the
combined tree does not establish that every future intermediate commit builds.
The shared harness can precede the firmware changes; add a CI group when its
production dependency is included.

Temporary builds are removed after each group. The optional `.clang-format`
file applies only to host C++ tests. It is not a firmware formatting change.

See [README_github_ota.md](README_github_ota.md) for the recorded release fixture, production-code coverage and fault-injection limits.

The status-memory fixtures are snapshots produced from the existing MAX/GK readings in `pi_fixture.h` using the real production parsers and snapshot methods. Identity and network fields use `Solar Test` and documentation IP addresses. The group extracts the real WebServerHandler helper, status builder, refresh method and data callback, plus SolarState snapshot code. Real ArduinoJson uses an accounting/fault allocator; String reserve/concat and WebSocket transport are controlled boundaries. Successful bodies are checked against the unchanged production serializer. This measures JSON bytes still live at transport, not total ESP32 heap or TLS/WiFi demand.

The group also checks cached status, response-copy failures, a full WebSocket queue and failed cache growth. An incomplete allocation must preserve the previous complete status or return HTTP 503, rather than publish a partial successful response.

The `firmware-writer` group compiles the complete updater and the actual manual upload callbacks. Its flash substitute follows the installed ESP32 `Update` ownership/error contract. It tests overlapping writers, empty or multiple files, allocation/write/finalize failures, disconnect cleanup and reservation until restart. These checks do not replace real image validation and device OTA tests.

The `snapshot-memory` group uses the existing MAX/GK status fixtures and the real
ArduinoJson allocator. It fails each allocation while replacing inverter/local
state, copying a response or rebuilding an overflowed working document. Rejected
transactions must retain the complete prior snapshot, clear failed output copies
and recover on a later successful attempt. Local energy sentinels come from the
existing restart tests; no additional inverter readings are invented.
Unchanged local status commits must allocate nothing. Changes, removals, additions
and swapping explicit null-valued local keys still require a complete transaction.
The same group compiles both temperature services and checks their current JSON
targets after working-document recovery, including value removal and republishing.

The timed PI integration group also rejects new battery timestamps after failed
snapshot commits, across MAX, GK, PI41 and REVO with/without QALL. Recovery needs
a completed fresh static pass and, for a previously known battery, a new battery
tuple after that pass. Modbus tests restart interrupted static/live passes,
invalidate the register cache, and discard pending/deferred battery updates.

Transport tests replay existing Q1/PV frames with delayed task scheduling. Buffered
bytes are consumed before idle timeout checks, while cancellation and the hard
total-frame deadline retain priority. Without byte-arrival timestamps, delayed
execution cannot distinguish timely buffered data from a physically late reply
within that total deadline. These tests do not make an absent/NAK inverter reply
valid or prove the cause of a past unrecorded hardware event.

The OTA/status groups also exercise the real nonblocking status guard. They check
mutex contention, updater startup ordering, skipped WebSocket allocation during
an update worker, continued REST responses and status delivery after it finishes.

The `snapshot-serialization` group compares the new direct API output byte for
byte with the former snapshot-and-serialize path using both existing device
fixtures. It checks local overlays, ordering, nulls, escaping, reserve/short-write
failures, lock release and a concurrent commit. Reported savings describe the
avoided JSON document, not total ESP32 heap or the HTTP response buffer.

The `discovery-memory` group denies the former scratch-vector reservation during
repeated ordinary discovery passes. It checks all seven real publication helpers,
late sensors and migration states, and confirms that forced reconciliation keeps
the same topic order and removes only stale configurations. This does not claim
that forced discovery or registration of a new topic needs no memory.
