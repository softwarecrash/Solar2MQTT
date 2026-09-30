(() => {
  const byId = (id) => document.getElementById(id);
  const sleep = (ms) => new Promise((resolve) => window.setTimeout(resolve, ms));
  const initial = new Map();

  const fields = [
    ["invOutputMode", "Output_Source_Priority", "outputmode"],
    ["invChargerPriority", "Charger_Source_Priority", "setting:chargerpriority"],
    ["invInputRange", "Input_Voltage_Range", "setting:inputrange"],
    ["invBatteryType", "Battery_Type", "batterytype"],
    ["invOutputFreq", "AC_Out_Rating_Frequency", "setting:outputfreq"],
    ["invOutputVoltage", "AC_Out_Rating_Voltage", "setting:outputvoltage"],
    ["invMaxCharge", "Current_Max_Charging_Current", "setting:maxcharge"],
    ["invUtilityCharge", "Current_Max_AC_Charging_Current", "setting:utilitycharge"],
    ["invRecharge", "Battery_Recharge_Voltage", "setting:recharge"],
    ["invRedischarge", "Battery_Redischarge_Voltage", "setting:redischarge"],
    ["invBulk", "Battery_Bulk_Voltage", "setting:bulk"],
    ["invFloat", "Battery_Float_Voltage", "setting:float"],
    ["invCutoff", "Battery_Under_Voltage", "setting:cutoff"],
    ["invEqVoltage", "Battery_Equalization_Voltage", "setting:equalizationvoltage"],
    ["invEqTime", "Battery_Equalization_Time", "setting:equalizationtime"],
    ["invEqTimeout", "Battery_Equalization_Timeout", "setting:equalizationtimeout"],
    ["invEqInterval", "Battery_Equalization_Interval", "setting:equalizationinterval"],
  ];

  const piSwitches = [
    ["invEqEnabled", "Battery_Equalization_Enabled", "powmr pi PBEQE1", "powmr pi PBEQE0"],
    ["invBuzzer", "Buzzer_Enabled", "powmr pi PEa", "powmr pi PDa"],
    ["invOverloadBypass", "Overload_Bypass_Enabled", "powmr pi PEb", "powmr pi PDb"],
    ["invPowerSaving", "Power_Saving_Enabled", "powmr pi PEj", "powmr pi PDj"],
    ["invLcdReset", "LCD_Reset_To_Default_Enabled", "powmr pi PEk", "powmr pi PDk"],
    ["invDataLogPopup", "Data_Log_Pop_Up", "powmr pi PEl", "powmr pi PDl"],
    ["invFeedToGrid", "Solar_Feed_To_Grid_Enabled", "powmr pi PEd", "powmr pi PDd"],
    ["invOverloadRestart", "Overload_Restart_Enabled", "powmr pi PEu", "powmr pi PDu"],
    ["invOverTempRestart", "Over_Temperature_Restart_Enabled", "powmr pi PEv", "powmr pi PDv"],
    ["invLcdBacklight", "LCD_Backlight_Enabled", "powmr pi PEx", "powmr pi PDx"],
    ["invPrimaryAlarm", "Primary_Source_Interrupt_Alarm_Enabled", "powmr pi PEy", "powmr pi PDy"],
    ["invRecordFault", "Record_Fault_Code_Enabled", "powmr pi PEz", "powmr pi PDz"],
  ];

  const normalize = (id, value) => {
    const text = value == null ? "" : String(value);
    if (id === "invOutputMode") {
      const map = { "Utility first": "UTI", "Solar first": "SUB", "SBU priority": "SBU", UTI: "UTI", SUB: "SUB", SBU: "SBU" };
      return map[text] || text;
    }
    if (id === "invChargerPriority") {
      const map = {
        "Utility first": "UTILITY",
        "Solar first": "SOLAR",
        "Solar and Utility": "SOLAR_UTILITY",
        "Solar only": "SOLAR_ONLY",
      };
      return map[text] || text;
    }
    if (id === "invInputRange") {
      const map = { Appliances: "APL", UPS: "UPS" };
      return map[text] || text;
    }
    if (id === "invOutputFreq") {
      return text.replace(/[^0-9.]/g, "") || text;
    }
    return text;
  };

  const toBool = (value) => {
    if (value === true) return true;
    const text = String(value ?? "").trim().toLowerCase();
    return text === "1" || text === "true" || text === "on" || text === "enabled";
  };

  const findValue = (data, key) => {
    const containers = [data?.DeviceData, data?.StaticData, data?.Settings, data?.LiveData, data];
    for (const container of containers) {
      if (container && Object.prototype.hasOwnProperty.call(container, key)) return container[key];
    }
    return null;
  };

  async function fetchJson(url, options) {
    const response = await fetch(url, options);
    if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);
    return response.json();
  }

  function setResult(text, isError = false) {
    const el = byId("inverterSettingsResult");
    if (!el) return;
    el.textContent = text || "-";
    el.style.whiteSpace = "pre-wrap";
    if (isError) el.dataset.error = "1";
    else delete el.dataset.error;
  }

  async function loadValues() {
    const status = await fetchJson("/api/status");
    if (!["MODBUS_POWMR", "MODBUS_POWMR_PI"].includes(status.protocol)) {
      throw new Error("This page is available only when MODBUS_POWMR or MODBUS_POWMR_PI is the active protocol.");
    }

    const hybridPi = status.protocol === "MODBUS_POWMR_PI";
    const note = byId("invPiSettingsNote");
    if (note) {
      note.textContent = hybridPi
        ? "PI+Modbus settings are available."
        : "These switches require PI+Modbus mode.";
    }

    const data = await fetchJson("/api/data");
    for (const [id, key] of fields) {
      const el = byId(id);
      if (!el) continue;
      const raw = findValue(data, key);
      if (raw == null) continue;
      const value = normalize(id, raw);
      el.value = value;
      initial.set(id, String(el.value));
    }

    for (const [id, key] of piSwitches) {
      const el = byId(id);
      if (!el) continue;
      const raw = findValue(data, key);
      const available = hybridPi && raw != null;
      el.disabled = !available;
      if (available) {
        el.checked = toBool(raw);
        initial.set(id, String(el.checked));
      } else {
        initial.delete(id);
      }
    }

    const active = data?.LiveData?.Battery_Equalization_Active;
    const activeEl = byId("invEqActive");
    if (activeEl) {
      activeEl.textContent = active == null ? "-" : (toBool(active) ? "ON" : "OFF");
    }

    setResult("Values loaded from inverter.");
  }

  const valuesMatch = (id, actual, expected) => {
    const a = normalize(id, actual);
    const e = normalize(id, expected);
    const an = Number(a);
    const en = Number(e);
    if (Number.isFinite(an) && Number.isFinite(en)) {
      return Math.abs(an - en) < 0.001;
    }
    return String(a).toLowerCase() === String(e).toLowerCase();
  };

  async function waitForValue(id, key, expected, timeoutMs = 8000) {
    const started = Date.now();
    let last = null;
    while (Date.now() - started < timeoutMs) {
      const data = await fetchJson("/api/data");
      const raw = findValue(data, key);
      if (raw != null) {
        last = normalize(id, raw);
        if (valuesMatch(id, last, expected)) return { matched: true, value: raw };
      }
      await sleep(300);
    }
    return { matched: false, value: last };
  }

  async function sendCommand(command) {
    const before = await fetchJson("/api/data");
    const previous = before?.RawData?.CommandAnswer || "";

    const body = new URLSearchParams();
    body.set("command", command);
    await fetchJson("/api/command", { method: "POST", body });

    const started = Date.now();
    let latest = "";
    while (Date.now() - started < 5000) {
      await sleep(180);
      const data = await fetchJson("/api/data");
      latest = data?.RawData?.CommandAnswer || "";
      if (latest && latest !== previous) break;
    }

    if (!latest) throw new Error(`No answer for command: ${command}`);
    if (latest.startsWith("OK:")) return latest;
    if (latest === "ACK") return `OK: ${command} (ACK)`;
    throw new Error(latest);
  }

  async function saveChanges(event) {
    event.preventDefault();
    const saveBtn = byId("inverterSaveBtn");
    if (saveBtn) saveBtn.disabled = true;

    const log = [];
    try {
      const changed = [];

      for (const [id, key, action] of fields) {
        const el = byId(id);
        if (!el) continue;
        const current = String(el.value);
        if (initial.get(id) === current) continue;
        changed.push({ type: "value", id, key, action, value: current });
      }

      for (const [id, key, onCommand, offCommand] of piSwitches) {
        const el = byId(id);
        if (!el || el.disabled || !initial.has(id)) continue;
        const current = String(el.checked);
        if (initial.get(id) === current) continue;
        changed.push({
          type: "switch",
          id,
          key,
          value: el.checked,
          command: el.checked ? onCommand : offCommand,
        });
      }

      if (!changed.length) {
        setResult("No changes.");
        return;
      }

      for (const item of changed) {
        let command = item.command;
        if (item.type === "value") {
          if (item.action === "outputmode") command = `powmr outputmode ${item.value}`;
          else if (item.action === "batterytype") command = `powmr batterytype ${item.value}`;
          else {
            const name = item.action.substring("setting:".length);
            command = `powmr setting ${name} ${item.value}`;
          }
        }

        const answer = await sendCommand(command);
        log.push(answer);

        const confirmed = await waitForValue(item.id, item.key, item.value);
        const el = byId(item.id);
        if (confirmed.matched) {
          if (el) {
            if (item.type === "switch") el.checked = toBool(confirmed.value);
            else el.value = normalize(item.id, confirmed.value);
          }
          initial.set(item.id, item.type === "switch"
            ? String(el?.checked ?? item.value)
            : String(el?.value ?? item.value));
        } else {
          // Keep the requested value visible when the inverter has accepted
          // the command but the next PI/Modbus refresh has not reached /api/data.
          initial.set(item.id, String(item.value));
          log.push(`Waiting for refreshed value: ${item.key}`);
        }
      }

      setResult("Saved:\n" + log.join("\n"));
    } catch (error) {
      log.push("ERROR: " + error.message);
      setResult(log.join("\n"), true);
    } finally {
      if (saveBtn) saveBtn.disabled = false;
    }
  }

  window.addEventListener("DOMContentLoaded", async () => {
    try {
      if (window.StatusBar && typeof window.StatusBar.connect === "function") {
        window.StatusBar.connect();
      }
    } catch (_) {}

    byId("inverterSettingsForm")?.addEventListener("submit", saveChanges);
    byId("inverterReloadBtn")?.addEventListener("click", async () => {
      try { await loadValues(); } catch (error) { setResult(error.message, true); }
    });

    try {
      await loadValues();
    } catch (error) {
      setResult(error.message, true);
    }
  });
})();
