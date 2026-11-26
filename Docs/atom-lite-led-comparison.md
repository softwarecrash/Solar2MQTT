# ATOM Lite addressable status LED trial

Measured on 2026-11-15 using clean ATOM Lite builds, platform 55.3.311,
Arduino ESP32 3.3.11, ESP-IDF libraries 5.5.5+sha.b774170ff46 and GCC 14.2.0+20260121.
Both reference and RGB builds completed after repairing the local package installation.

## Configuration

```ini
-DPIN_LED_STATUS=27
-DSTATUS_LED_ADDRESSABLE=1
```

`PIN_LED_STATUS` supplies the default GPIO. `STATUS_LED_ADDRESSABLE=1` selects
one GRB RGB LED (WS2812/SK6812-style); omitted or `0` selects a monochrome PWM LED.
The device settings still control the actual GPIO and brightness. GPIO `-1`
and brightness `0` disable the light. Existing saved GPIO `-1` settings remain
disabled: set Status LED GPIO to 27 to test an already configured ATOM Lite.

GPIO27 and the GRB ordering follow the [M5Stack documentation](https://docs.m5stack.com/en/homeassistant/devices/atom_lite).
The trial emits green pulses and preserves the existing 1/2/3/4 pulse mapping,
five-second cycle, fades, 20 ms task period and 3072-byte task stack allocation.
No additional library or task is added. Other board environments keep PWM.

## Build comparison

The reference uses the existing PWM backend and the previous ATOM Lite default
GPIO `-1`. The trial replaces that backend with RGB and defaults to GPIO27.
PlatformIO Flash means its normal program-size check; static RAM includes data,
bss and noinit, not heap allocations or task stacks. IRAM is a separate view of
code stored in RAM, not an additional amount to add to the Flash column.

| Metric (bytes) | Reference | RGB trial | Difference |
| --- | ---: | ---: | ---: |
| Flash/program | 1586515 | 1579184 | -7331 |
| Static RAM | 54456 | 54252 | -204 |
| IRAM code | 95727 | 95187 | -540 |
| Firmware .bin | 1628784 | 1621456 | -7328 |

The RGB build no longer links the PWM LEDC backend used by the old status LED
service; its net program footprint is smaller. This is a backend replacement
comparison, not a claim that an RMT driver requires no memory. The early trial
which retained both PWM and RGB backends instead added roughly 4 KB of program
storage; it was replaced by the simpler per-board type selection above.

## Work and validation

The host regression compiles the production service in three modes (flag absent,
0 and 1), checks disabling, brightness, pin switching, initialization failure,
resource release, task reuse and suppression of identical RGB frames.
At brightness 128, a simulated five-second production polling cycle sends:

| Pulses | Frames per 5 seconds | Average frames/second |
| --- | ---: | ---: |
| 1 | 27 | 5.4 |
| 2 | 53 | 10.6 |
| 3 | 79 | 15.8 |
| 4 | 105 | 21.0 |

The Arduino helper uses a 24-symbol (96-byte) temporary frame buffer and one RMT
TX channel. Its signal timing is about 28.8 microseconds per frame, excluding
software/driver overhead. These are source/scheduling observations, not measured
CPU percentages. Additional RMT heap use and task peak stack use were not measured.
No serial device was available for a physical LED, CPU or heap test; not flashed.

Run `node tests/host/run_status_led.cjs` or `node tests/host/run.cjs all`.
Local comparison ELF files and metrics are in `.pio/led-comparison/`.
