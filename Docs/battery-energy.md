# Local battery-energy counters

This change preserves the existing MQTT topics, Home Assistant unique IDs and
the units of `Battery_Charge_Energy` / `Battery_Discharge_Energy` (kWh). It does
not rename PV fields, change Q1/QEX parsing or change settings import/export.

## Storage and diagnostics

`GET /api/debug/energy` uses the existing web authentication. It reports the
actual running/next app, `backlog` and NVS partitions, selected storage backend,
checkpoint sequence, pending/dirty state, write errors, migration receipt and
battery-sample timing. It reads a cached snapshot; the HTTP callback does not
access flash or change inverter state.

App-only OTA does not create a missing filesystem partition. On layouts such as
the old `min_no_spiffs.csv`, the battery counters therefore use the existing NVS
partition through Preferences. A mounted `backlog` LittleFS partition continues
to serve the battery counters and the separate PV/AC offline backlog.

- A/B checkpoints include a checksum and are read back after writing. A failed
  write retries the other slot without overwriting the last verified slot.
  Disk totals round upward to one milliWh so a deliberate restart does not
  create a tiny apparent decrease through rounding; this adds less than one
  milliWh per counter when a checkpoint is loaded.
- The old 32-byte battery files are imported without deleting them. Valid
  counters survive missing `initialized` metadata.
- A newly erased partition can be formatted only after every byte was verified
  as `0xff`. A nonempty damaged or unreadable filesystem is not autoformatted.
- If the filesystem is inaccessible at startup, recovery in NVS is marked
  persistently. Later reappearance of stale filesystem checkpoints cannot
  replace the recovered totals. This recovery stays on NVS.
- A write failure on an already mounted filesystem can fall back to NVS using
  the shared sequence. The newest valid record is recovered on the next boot.
- Normal checkpoints occur every 120 seconds; failed attempts retry no faster
  than every 10 seconds. Existing deliberate-restart/OTA hooks flush the held
  reading. An abrupt power cut can still lose energy since the last checkpoint.
- Only the explicit factory-reset marker authorizes clearing checkpoints.
  Invalid existing records are retained for manual recovery rather than
  silently replaced by zero.

The inverter PV/AC offline backlog format and its eight existing energy keys
are unchanged. There is no filesystem-backed offline backlog on layouts that
have no filesystem; NVS fallback here applies to the two local battery counters.

## Time integration

The previous accepted voltage/current reading is held until the next complete
sample is accepted, and its actual interval is integrated in Wh. Publication
still uses the existing calculated-power names and kWh counter names. Removing
the one-second integration gate prevents phase-dependent loss of short phases
that are present in the accepted readings.

The freshness lease includes five seconds of cycle jitter reserve and is capped
at 120 seconds. It is frozen with each battery sample: warning/status-only
replies cannot extend an old battery reading indefinitely. Integration clips an
interval at lease expiration. Invalid numeric samples do not add energy.

Acceptance time is when a complete inverter snapshot becomes available to the
application. The acquisition time separately limits freshness. This improves
integration of the observed sample stream; it cannot measure physical current
changes between inverter readings or remove the inverter's own quantization.
An independent meter remains necessary to verify physical energy accuracy.

Known read-only PI diagnostic commands wait for a completed normal polling
cycle. At most one queued inquiry runs between complete cycles, and it does
not restart static initialization. Its reply remains a command answer, not a
new battery reading; real expiry and communication failures still stop energy
integration. Settings and unknown commands keep their existing refresh path.
Native Modbus and explicit PowMr PI command scheduling are unchanged.

## First upgrade from RAM-only counters

Do this per device during a separately authorized OTA. The tool below does not
upload firmware or restart a device. Do not combine earlier counter segments
that HA may already have counted through previous resets.

1. Immediately before OTA, capture the **currently visible** two counters from
   the old firmware. Supply its existing HA MQTT device identifier. Authentication
   can be supplied using `SOLAR2MQTT_USER` / `SOLAR2MQTT_PASSWORD` environment
   variables. Credentials are never saved in the capture.

   ```sh
   node tools/battery_energy_migration.cjs capture --url http://192.0.2.25 --device-id solar2mqtt_AABBCCDDEEFF --out battery-25-before-ota.json
   ```

2. Review the capture and retain it. `capture` only performs GET requests and
   refuses absent/nonnumeric totals or a restart during capture. Output files
   are created without overwriting earlier captures.

3. After the separately authorized firmware update, inspect
   `/api/debug/energy`. A legacy device with no valid durable checkpoint enters
   `migration.pending=true`. It collects and checkpoints the new session while
   withholding provisional counter totals. Existing HA Discovery identities
   and retained counter topics are preserved. A valid existing checkpoint is
   loaded automatically and requires no baseline addition.

4. Prepare a reviewable request without sending it:

   ```sh
   node tools/battery_energy_migration.cjs prepare --snapshot battery-25-before-ota.json --out battery-25-request.json
   ```

   Preparation checks device ID, hostname, MQTT topic, storage availability,
   migration state and current boot ID. It performs only GET requests. If the
   IP changed, `--url` can target the new origin while these identity checks
   remain enforced.

5. Restore using the **same capture**. This command submits the prepared baseline:

   ```sh
   node tools/battery_energy_migration.cjs restore --snapshot battery-25-before-ota.json
   ```

   The device adds the captured visible baseline to its new session, and writes
   the result together with the request receipt. It publishes the same old MQTT
   names only after verification. A repeated request, including after reboot,
   cannot add the baseline again. A lost acknowledgement or failed write must
   be retried using the same capture, not a newly generated request ID.

6. Compare diagnostics, MQTT and HA counters; verify the HA entities and their
   prior history still exist. Then verify another deliberate restart. No HA
   database rewrite or entity recreation is part of this procedure.

The last capture and the old firmware shutdown cannot be atomic with the old
RAM-only implementation. Energy after that last capture and before restart
cannot be reconstructed reliably. Capture as late as possible; do not invent
an offset for the gap. Older already-lost sessions require a separate audited
HA-history analysis and are not added by this tool.

If all checkpoint copies are corrupt, the server requires explicit recovery
acknowledgement; the tool exposes `--acknowledge-recovery` for that reviewed
case. Its purpose is to prevent automatic replacement of unknown stored totals.

### First initialization without an old counter

Version 2.0.4 did not publish these local battery-energy counters. If no old
baseline exists and diagnostics report `migration.pending=true`, prepare an
explicit zero-baseline initialization. Use the actual device ID from
`/api/debug/energy`; the ID below is a placeholder.

```sh
node tools/battery_energy_migration.cjs prepare-zero --url http://192.0.2.25 --device-id solar2mqtt_AABBCCDDEEFF --out battery-zero.json
```

This performs only GET requests and creates a file without overwriting an
existing file. It records the device identity, request ID, zero baselines and
the observed new-session totals. Review and keep that file, then apply it:

```sh
node tools/battery_energy_migration.cjs start-zero --snapshot battery-zero.json
```

This submits the existing `start_zero` / `start_new_series` API operation and
waits for verified completion. Zero means **no old baseline is added**: energy
already counted in the pending new session remains. It does not reset a
running initialized counter. A captured known baseline must use `restore`.

After an uncertain response, timeout or reboot, retry using the same file.
Do not prepare a new request ID. The existing receipt makes retries
idempotent. Initialized counters, another device/topic, simulation, unavailable
storage and unresolved writes are rejected. Corrupt-record recovery still
requires the separately reviewed `--acknowledge-recovery` option on preparation
and application. No firmware/settings/reboot commands are issued by this tool.

`prepare --snapshot battery-zero.json --out request-preview.json` can display
the API payload without submitting it. Authentication uses the same environment
variables as capture/restore and is never written into either output file.

New checkpoints use a versioned 48-byte format so receipt and counters remain
one record. The retained 32-byte files are an upgrade source, not a live mirror.
Downgrading to a firmware that only reads the old format can expose stale or
zero local counters; capture the current totals before any such downgrade.

## Validation

Modbus battery freshness is based on successful reads of all required battery
registers in a completed live pass. Block-cache acquisition time is retained;
static/status successes and partial battery failures cannot refresh an old
sample. Its validity allowance covers two passes plus 5 seconds, capped at
6 minutes for the longest supported profile at a 5-second poll interval.
PI readers keep their existing 120-second cap. These are measurement freshness
limits; the automatic checkpoint interval remains 120 seconds.

GitHub OTA requests a restart from the main loop, using the same energy flush
and inverter shutdown path as a requested API restart. A failed checkpoint is
reported in the existing log/diagnostics; storage failure still cannot promise
preservation of uncheckpointed energy.

After successful baseline restoration or explicit zero-start, normal discovery
republishes only the two reserved energy configurations with the existing
topics and unique IDs. It does not delete retained configurations or rename
entities; repeated successful requests do not cause repeated discovery sends.

Run the commands in `tests/host/README_state_cycles.md`, plus the QPIGS/QPIGS2,
PI serial, PowMr PV2 and PV-total regression checks. The host tests compile the
production persistence/integration code with fault-injected storage APIs and
replay MAX/24, PIP-GK/25, PI41 and REVO polling. They do not prove physical NVS
durability, actual partition layouts or independent meter accuracy. Those
checks need the later authorized hardware test.
