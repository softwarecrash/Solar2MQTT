Release Notes

- 2.0.18P completes PowMr PI+Modbus configuration handling in the built-in Web UI.
- The inverter settings page now exposes battery equalization enable/disable plus all QFLAG settings as real toggle switches and shows whether equalization is currently active.
- Web switches are available only when `MODBUS_POWMR_PI` is active and are disabled in pure Modbus mode.
- PI write replies using `ACK` are treated as successful Web UI commands; the page then waits for the refreshed inverter state before confirming the switch.
- Home Assistant keeps only automation-relevant PowMr switches: battery equalization, overload bypass, power saving, overload restart, over-temperature restart, and solar feed-to-grid.
- Panel/service-only flags are removed from HA Discovery and remain configurable in the Web UI: buzzer, LCD reset, LCD backlight, data-log pop-up, primary-source interrupt alarm, and fault-code recording.
- Existing numeric/select inverter settings and 2.0.18O post-write refresh behavior are unchanged.

- 2.0.18O fixes stale values shown by the PowMr inverter settings Web UI immediately after a successful write.
- After each accepted setting change the page now polls /api/data until the requested value is actually present in refreshed DeviceData, instead of reloading the old cached value after 400 ms.
- If the inverter state refresh takes longer than the confirmation window, the requested value remains visible and the UI reports that state refresh is still pending instead of visually reverting to the previous value.
- This complements the 2.0.18N mirror-register verification fix; no inverter protocol behavior is changed.

- 2.0.18N adds explicit battery equalization control for the PowMr PI+Modbus mode.
- `QBEQI` is polled in hybrid mode; Home Assistant receives «Выравнивание АКБ» as a configuration switch and «Выравнивание АКБ активно» as the current equalization activity state.
- The switch uses the PI30 commands `PBEQE1` / `PBEQE0` and refreshes `QBEQI` after a write.
- PowMr 50xx setting writes are now verified through the readable 45xx mirror registers instead of reading the control register back directly. For example, equalization voltage writes to 5030 are verified through 4549.
- Mirror readback is retried; if the write was acknowledged but immediate mirror verification is unavailable, the UI reports a successful accepted write with pending refresh instead of a false write error.
- Output-source priority and battery-type verification use the same mirrored-register path.

- 2.0.18M completes the explicit PowMr PI+Modbus protocol integration in the Web UI.
- The PowMr settings page now accepts both `MODBUS_POWMR` and `MODBUS_POWMR_PI`.
- Existing `MODBUS_POWMR`, PI30/PI41 and other protocols remain unchanged.
- `MODBUS_POWMR_PI` remains manual-only and is never selected by autodetect.
- Russian Web UI text now explains the manual hybrid-mode requirement.

- 2.0.18L adds an explicit manual-only `PI+Modbus — PowMr HVM` protocol mode (`MODBUS_POWMR_PI`).
- The existing `MODBUS_POWMR` mode is restored to pure Modbus behavior; other inverter protocols are unchanged.
- Hybrid PI+Modbus is never selected by autodetect and must only be chosen for a compatible PowMr exposing Modbus and PI30 on the same UART.
- Modbus remains authoritative. PI30 supplement queries only add selected values not available from the verified PowMr Modbus register map.
- Hybrid supplement reads selected QPIRI values including rated active/apparent power, rated current/voltage, machine type, parallel limit, output mode, operation logic, PV balance and discharge limit.
- Hybrid supplement reads selected QPIGS values including inverter bus voltage/temperature, PV input current, SCC battery voltage, EEPROM version, PV charging power and device status.
- Q1 supplement keeps temperatures, fan data and charge state without overwriting the Modbus inverter temperature.
- QFLAG values are exposed as Home Assistant configuration switches only in hybrid mode: buzzer, overload bypass/restart, over-temperature restart, power saving, LCD reset/backlight, data-log popup, primary-source alarm, fault recording and feed-to-grid.
- PI-only writes use the existing `powmr pi` transport and are accepted only while the explicit hybrid protocol is selected.
- Hybrid PI requests are rate-limited to at most one request per complete Modbus pass.

- 2.0.18K makes Home Assistant cleanup protocol-aware for MODBUS_POWMR.
- Removes stale retained PI30/QPIRI/QPIGS entities that are not updated by the active PowMr Modbus protocol and could show old English names or conflicting values.
- Keeps the native PowMr Modbus telemetry, Q1 supplemental temperature/fan/charge-state data, dedicated PowMr select/number settings, and native battery power/energy counters.
- Does not change entity_id/unique_id for supported current entities.

- 2.0.18J completes the Home Assistant entity migration started in 2.0.18I.
- Explicitly removes retained discovery for obsolete PowMr debug/status entities: `PowMr_Debug_4556`, `PowMr_Debug_4558`, `PowMr_Debug_4559`, `PowMr_Debug_4560`, `PowMr_Debug_4561`, `PowMr_Status_Flags_1`, `PowMr_Status_Flags_2`, and `PowMr_Settings_Flags`.
- Explicitly removes legacy generic sensor/binary_sensor duplicates for PowMr settings now represented by dedicated select/number entities.
- Runs a second forced Home Assistant Discovery refresh 10 seconds after MQTT connection so static DeviceData entities are republished with Russian display names after inverter static polling completes.
- Keeps the useful `Solar_Feed_To_Grid_Enabled` entity and publishes it as «Разрешение отдачи в сеть».
- Existing entity_id/unique_id values for supported entities are preserved.

- 2.0.18I cleans up Home Assistant MQTT Discovery and localizes Solar2MQTT entity names to Russian.
- All explicitly supported static/live Solar2MQTT sensors now publish Russian display names while existing entity_id/unique_id values remain unchanged.
- ESP32 internal temperature and DS18B20 discovery names are localized to Russian.
- PowMr control entity names are localized while command/state payload compatibility is preserved.
- Generic HA discovery now publishes only explicitly catalogued entities; runtime diagnostic/probe fields are no longer exposed as sensors.
- On MQTT reconnect the firmware performs a short retained Discovery sweep scoped to its own device and removes stale entities that are not part of the current supported catalog, including old debug/probe entities no longer present in runtime data.
- Battery charging/discharging power and cumulative battery energy sensors from 2.0.18H are preserved unchanged.

- 2.0.18H adds native cumulative battery charge/discharge energy counters and fixes Home Assistant long-term statistics for live physical measurements published through MQTT Discovery.
- Native `Зарядка батареи` / `Разрядка батареи` sensors are published in kWh with `device_class: energy` and `state_class: total_increasing`; their requested entity IDs are `sensor.zariadka_batarei` and `sensor.razriadka_batarei` for migration from existing HA helpers without losing Recorder statistics.
- Battery energy is integrated locally from actual battery voltage and net battery current, so it no longer depends on a fixed 48 V template.
- Native calculated `sensor.battery_charging_power` and `sensor.battery_discharging_power` replace the previous HA template power sensors and use actual battery voltage.
- The counters continue accumulating while MQTT is offline, survive ESP32 restarts, use alternating verified LittleFS checkpoints, and are flushed before planned restarts.
- Checkpoints are also written when battery flow stops or changes direction, limiting flash wear while keeping the totals durable.
- Battery SOC now publishes `state_class: measurement`, so `sensor.solar2mqtt_battery_percent` can be selected as battery state of charge in the Home Assistant Energy dashboard.
- The same statistics metadata is added consistently to live voltage, current, power, apparent-power, frequency and temperature sensors.

- 2.0.18G adds persistent LittleFS storage for cumulative PV/grid energy counters while MQTT is offline.
- Saved counter samples survive ESP32 reboots and are replayed in order to the normal `LiveData/*` MQTT topics after reconnect.
- Backlog replay is batched, retained topics are finalized with the current live snapshot, and the backlog file is deleted only after a complete successful replay.
- Offline writes are compact binary snapshots, throttled to five-minute intervals (plus immediate offline entry and counter reset detection) to reduce flash wear.
- All supported ESP32 targets now use the shared dual-OTA + 384 KiB LittleFS backlog partition layout.

- Web UI now loads full HTML pages without server-side placeholders; dynamic data comes from `/meta` and `/config`.
- Debug page adds a serial loopback test and a downloadable debug report for current raw/parsed data.
- Favicon is served as `favicon.ico` and the GitHub update check is cached to reduce browser/heap load.
- WebSocket handling and page streaming were optimized to lower heap usage and reduce random page load failures.
- Protocol parsing was expanded to better handle variable inverter response lengths.
