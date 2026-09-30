# PowMr SOC register correlation (2.0.18E)

Read-only FC03 (holding) and FC04 (input) diagnostics. Commands never write to
inverter registers. Address numbers are the actual wire addresses, not 4xxxx
human-readable Modbus references.

## Procedure

Keep battery type LIL and the upper threshold at 70%. Set the lower threshold
to 30% before capture, and do not change settings while capture is running.

- `powmr hunt baseline`: discover both register spaces over 0..65535. Starts
  at address 4000, alternates FC03/FC04 per address, wraps around through zero.
  This is exhaustive probing, potentially many hours, not a quick live snapshot.
- `powmr hunt baseline 4000 6000`: optional bounded first pass, 4002 probes.
- After completion, change only 30 -> 35 on the inverter and save; wait 15 s.
- `powmr hunt compare`: reread discovered addresses, compare against the fixed
  baseline. Nothing replaces the baseline automatically.
- Return to 30 and run compare again to verify the same registers return.
- `powmr hunt result` or `powmr hunt result 1`: retrieve saved results, 40 rows
  per zero-based page. Comparison pages show changes and read errors only.
- `powmr hunt status`: progress; reopening the web page and running status
  resumes waiting without restarting discovery.
- `powmr hunt cancel`: stop, retaining a clearly marked partial baseline/result.

The console waits for completion and has a stop button. It polls status, never
re-sends baseline/compare. Normal Modbus telemetry continues between individual
scan reads, at a reduced rate. Each scan read has a 500 ms timeout; transport
errors get one retry on a later step. Modbus exceptions are counted separately.

## Limits and interpretation

4096 readable registers maximum (32 KiB). Hitting the limit stops with
CAPACITY_LIMIT, never silently skips values. Use narrower explicit ranges to
investigate other banks. Baseline and results are RAM-only and lost on reboot.
A new baseline explicitly replaces the old one. Save/paste relevant results
before starting another baseline. Baseline values span the scan duration,
not one instant. Keep threshold settings constant for the whole scan.

DONE means every requested address/function was attempted; rejected addresses
and transport errors are still reported. Transport errors leave coverage
uncertain. Cancelled/capacity-limited baselines also have incomplete coverage.
Comparisons revisit only successful baseline addresses: a setting exposed
only after a mode change would require a new discovery. Never treat missing
responses as zero, or assume an incidental value of 30/70 identifies a setting.

Legacy `powmr watch` now also keeps its 64-register baseline fixed.
`powmr watch reset` explicitly captures a replacement baseline; failed reads
leave the old baseline untouched.

## Host verification

`g++ -std=c++17 -Wall -Wextra -Werror tests/powmr_hunt_test.cpp -o /tmp/hunt-test && /tmp/hunt-test`

Tests cover all 131072 address/function pairs (including 65535), fixed baseline
across both directions, retries, unread markers, capacity and cancellation.
Hardware verification requires the actual inverter and is not claimed here.
