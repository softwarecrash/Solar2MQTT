const state = {
  statusSocket: null,
  toastTimer: null,
  previewFocusTimers: {},
};

function byId(id) {
  return document.getElementById(id);
}

function setText(id, value) {
  const node = byId(id);
  if (node) {
    node.textContent = value ?? "-";
  }
}

function hasAny(ids) {
  return ids.some((id) => byId(id));
}

function formatValue(value, suffix = "") {
  if (value === null || value === undefined || value === "") {
    return "-";
  }
  return `${value}${suffix}`;
}

function showNotice(message, isError = false) {
  const toast = byId("toast");
  const toastIcon = byId("toast-icon");
  const toastMessage = byId("toast-msg");

  if (toast && toastMessage) {
    toastMessage.textContent = message;
    if (toastIcon) {
      toastIcon.textContent = isError ? "!" : "OK";
    }
    toast.style.borderColor = isError ? "var(--ea-warning)" : "var(--ea-primary)";
    toast.classList.add("show");
    if (state.toastTimer) {
      window.clearTimeout(state.toastTimer);
    }
    state.toastTimer = window.setTimeout(() => {
      toast.classList.remove("show");
    }, 2800);
  }

  const noticeNodes = document.querySelectorAll("[data-notice]");
  if (!noticeNodes.length) {
    if (!toast) {
      window.alert(message);
    }
    return;
  }

  noticeNodes.forEach((node) => {
    node.hidden = false;
    node.textContent = message;
    node.dataset.state = isError ? "error" : "ok";
  });
}

function clearNotice() {
  const toast = byId("toast");
  if (toast) {
    toast.classList.remove("show");
  }

  document.querySelectorAll("[data-notice]").forEach((node) => {
    node.hidden = true;
    node.textContent = "";
    node.dataset.state = "";
  });
}

function fillForm(formId, values) {
  const form = byId(formId);
  if (!form || !values) {
    return;
  }

  Object.entries(values).forEach(([key, value]) => {
    const field = form.elements.namedItem(key);
    if (field) {
      if (field instanceof RadioNodeList) {
        field.value = value ?? "";
      } else if (field.type === "checkbox") {
        field.checked = value === true || value === 1 || value === "1";
      } else {
        field.value = value ?? "";
      }
    }
  });
}

const OVERVIEW_GROUPS = [
  {
    title: "Solar",
    fields: [
      { label: "V", keys: ["PV_Input_Voltage", "PV1_Input_Voltage"], unit: "V", decimals: 1 },
      { label: "A", keys: ["PV_Input_Current"], unit: "A", decimals: 1 },
      { label: "W", keys: ["PV_Input_Power", "PV1_Input_Power", "PV_Charging_Power"], unit: "W", decimals: 0 },
    ],
  },
  {
    title: "Solar 2",
    fields: [
      { label: "V", keys: ["PV2_Input_Voltage"], unit: "V", decimals: 1 },
      { label: "A", keys: ["PV2_Input_Current"], unit: "A", decimals: 1 },
      { label: "W", keys: ["PV2_Input_Power", "PV2_Charging_Power"], unit: "W", decimals: 0 },
    ],
  },
  {
    title: "Grid",
    fields: [
      { label: "V", keys: ["AC_In_Voltage"], unit: "V", decimals: 1 },
      { label: "Hz", keys: ["AC_In_Frequency"], unit: "Hz", decimals: 1 },
    ],
  },
  {
    title: "Grid L2",
    fields: [
      { label: "V", keys: ["AC_In_Voltage_L2"], unit: "V", decimals: 1 },
      { label: "Hz", keys: ["AC_In_Frequency_L2"], unit: "Hz", decimals: 1 },
    ],
  },
  {
    title: "AC Out",
    fields: [
      { label: "V", keys: ["AC_Out_Voltage", "AC_Output_Voltage"], unit: "V", decimals: 1 },
      { label: "Hz", keys: ["AC_Out_Frequency", "AC_Output_Frequency"], unit: "Hz", decimals: 1 },
      { label: "W", keys: ["AC_Out_Watt", "AC_Output_Power", "Output_Power"], unit: "W", decimals: 0 },
      { label: "%", keys: ["AC_Out_Percent", "Output_Load_Percent"], unit: "%", decimals: 0 },
      { label: "A", keys: ["AC_Output_Current", "Output_Current"], unit: "A", decimals: 1 },
    ],
  },
  {
    title: "AC Out L2",
    fields: [
      { label: "V", keys: ["AC_Out_Voltage_L2"], unit: "V", decimals: 1 },
      { label: "Hz", keys: ["AC_Out_Frequency_L2"], unit: "Hz", decimals: 1 },
      { label: "W", keys: ["AC_Out_Watt_L2"], unit: "W", decimals: 0 },
      { label: "%", keys: ["AC_Out_Percent_L2"], unit: "%", decimals: 0 },
    ],
  },
  {
    title: "Battery",
    fields: [
      { label: "V", keys: ["Battery_Voltage", "Positive_Battery_Voltage"], unit: "V", decimals: 1 },
      { label: "A", keys: ["Battery_Load"], unit: "A", decimals: 1 },
      { label: "Chg", keys: ["Battery_Charge_Current"], unit: "A", decimals: 1 },
      { label: "Dis", keys: ["Battery_Discharge_Current"], unit: "A", decimals: 1 },
      { label: "%", keys: ["Battery_Percent"], unit: "%", decimals: 0 },
    ],
  },
  {
    title: "Temperature",
    fields: [
      { label: "Inv", keys: ["Inverter_Temperature", "Inverter_Bus_Temperature"], unit: "C", decimals: 0 },
      { label: "Bat", keys: ["Battery_Temperature"], unit: "C", decimals: 0 },
      { label: "Track", keys: ["Tracker_Temperature", "MPPT1_Charger_Temperature"], unit: "C", decimals: 0 },
      { label: "Trf", keys: ["Transformer_Temperature"], unit: "C", decimals: 0 },
    ],
  },
  {
    title: "Inverter",
    fields: [
      { label: "Mode", keys: ["Inverter_Operation_Mode"], numeric: false },
      { label: "Charge", keys: ["Inverter_Charge_State"], numeric: false },
    ],
  },
];

function isDataValuePresent(value) {
  if (value === null || value === undefined) {
    return false;
  }

  if (typeof value === "string") {
    const trimmed = value.trim();
    if (!trimmed) {
      return false;
    }

    const lowered = trimmed.toLowerCase();
    return lowered !== "null" && lowered !== "undefined" && lowered !== "nan" && lowered !== "-";
  }

  return true;
}

function parseNumber(value) {
  if (!isDataValuePresent(value)) {
    return null;
  }

  const number = Number(value);
  return Number.isFinite(number) ? number : null;
}

function formatNumber(value, decimals = 0) {
  const number = parseNumber(value);
  if (number === null) {
    return null;
  }

  if (decimals <= 0) {
    return String(Math.round(number));
  }

  return number
    .toFixed(decimals)
    .replace(/\.0+$/, "")
    .replace(/(\.\d*?[1-9])0+$/, "$1");
}

function formatReading(value, unit = "", decimals = 0) {
  if (!isDataValuePresent(value)) {
    return null;
  }

  const numericText = formatNumber(value, decimals);
  const text = numericText !== null ? numericText : String(value).trim();
  return unit ? `${text} ${unit}` : text;
}

function pickDataValue(data, keys, sections = ["LiveData", "DeviceData", "EspData", "Status"]) {
  const keyList = Array.isArray(keys) ? keys : [keys];

  for (const sectionName of sections) {
    const section = data?.[sectionName];
    if (!section || typeof section !== "object") {
      continue;
    }

    for (const key of keyList) {
      const value = section[key];
      if (isDataValuePresent(value)) {
        return value;
      }
    }
  }

  return null;
}

function pickDataNumber(data, keys, sections) {
  return parseNumber(pickDataValue(data, keys, sections));
}

function clampPercent(value) {
  const number = parseNumber(value);
  if (number === null) {
    return null;
  }

  return Math.max(0, Math.min(100, number));
}

function ratioToPercent(current, maximum) {
  const currentNumber = parseNumber(current);
  const maximumNumber = parseNumber(maximum);
  if (currentNumber === null || maximumNumber === null || maximumNumber <= 0) {
    return null;
  }

  return clampPercent((currentNumber / maximumNumber) * 100);
}

function joinMeta(parts) {
  return parts.filter(Boolean).join(" | ");
}

function totalSolarPower(data) {
  const chargingPower = pickDataNumber(data, ["PV_Charging_Power", "SCC_Charge_Power"], ["LiveData"]);
  if (chargingPower !== null) {
    return chargingPower;
  }

  const pv1Power = pickDataNumber(data, ["PV_Input_Power", "PV1_Input_Power"], ["LiveData"]);
  const pv2Power = pickDataNumber(data, ["PV2_Input_Power", "PV2_Charging_Power"], ["LiveData"]);

  if (pv1Power !== null && pv2Power !== null) {
    return pv1Power + pv2Power;
  }

  return pv1Power ?? pv2Power;
}

function setMeter(name, valueText, percent, metaText) {
  const card = byId(`meter${name}Card`);
  const valueNode = byId(`meter${name}Value`);
  const barNode = byId(`meter${name}Bar`);
  const metaNode = byId(`meter${name}Meta`);

  if (!card || !valueNode || !barNode || !metaNode) {
    return;
  }

  const normalizedPercent = clampPercent(percent);
  const hasValue = isDataValuePresent(valueText);

  valueNode.textContent = hasValue ? valueText : "-";
  metaNode.textContent = metaText || "No live data yet";
  barNode.style.width = `${normalizedPercent ?? 0}%`;
  barNode.textContent = normalizedPercent === null || normalizedPercent < 14 ? "" : `${Math.round(normalizedPercent)}%`;
  card.dataset.state = hasValue || normalizedPercent !== null ? "ready" : "empty";
}

function renderMeters(data) {
  if (!hasAny(["meterBatteryCard", "meterLoadCard", "meterSolarCard", "meterChargeCard"])) {
    return;
  }

  const batteryPercent = pickDataNumber(data, ["Battery_Percent"], ["LiveData"]);
  const batteryVoltage = pickDataValue(data, ["Battery_Voltage", "Positive_Battery_Voltage"], ["LiveData"]);
  const batteryLoad = pickDataValue(data, ["Battery_Load"], ["LiveData"]);
  const chargeCurrent = pickDataNumber(data, ["Battery_Charge_Current"], ["LiveData"]);
  const dischargeCurrent = pickDataValue(data, ["Battery_Discharge_Current"], ["LiveData"]);
  const loadPercent =
    pickDataNumber(data, ["AC_Out_Percent", "Output_Load_Percent"], ["LiveData"]) ??
    ratioToPercent(
      pickDataNumber(data, ["AC_Out_Watt", "AC_Output_Power", "Output_Power"], ["LiveData"]),
      pickDataNumber(data, ["AC_Out_Rating_Active_Power"], ["DeviceData"])
    );
  const loadPower = pickDataValue(data, ["AC_Out_Watt", "AC_Output_Power", "Output_Power"], ["LiveData"]);
  const loadVoltage = pickDataValue(data, ["AC_Out_Voltage", "AC_Output_Voltage"], ["LiveData"]);
  const solarPower = totalSolarPower(data);
  const solarPercent = ratioToPercent(
    solarPower,
    pickDataNumber(data, ["AC_Out_Rating_Active_Power"], ["DeviceData"])
  );
  const solarVoltage = pickDataValue(data, ["PV_Input_Voltage", "PV1_Input_Voltage"], ["LiveData"]);
  const solarCurrent = pickDataValue(data, ["PV_Input_Current"], ["LiveData"]);
  const chargePower = pickDataValue(data, ["SCC_Charge_Power", "PV_Charging_Power"], ["LiveData"]);
  const maxChargeCurrent = pickDataNumber(data, ["Current_Max_Charging_Current"], ["DeviceData"]);
  const chargePercent = ratioToPercent(chargeCurrent, maxChargeCurrent);

  setMeter(
    "Battery",
    batteryPercent !== null ? `${Math.round(batteryPercent)} %` : formatReading(batteryVoltage, "V", 1),
    batteryPercent,
    joinMeta([
      formatReading(batteryVoltage, "V", 1),
      formatReading(batteryLoad, "A", 1),
      chargeCurrent !== null ? `Charge ${formatReading(chargeCurrent, "A", 1)}` : null,
    ]) || "Battery data unavailable"
  );

  setMeter(
    "Load",
    loadPercent !== null ? `${Math.round(loadPercent)} %` : formatReading(loadPower, "W", 0),
    loadPercent,
    joinMeta([
      formatReading(loadPower, "W", 0),
      formatReading(loadVoltage, "V", 1),
    ]) || "Load data unavailable"
  );

  setMeter(
    "Solar",
    solarPower !== null ? `${Math.round(solarPower)} W` : formatReading(solarVoltage, "V", 1),
    solarPercent,
    joinMeta([
      formatReading(solarVoltage, "V", 1),
      formatReading(solarCurrent, "A", 1),
    ]) || "Solar data unavailable"
  );

  setMeter(
    "Charge",
    chargeCurrent !== null ? `${formatNumber(chargeCurrent, 1)} A` : formatReading(chargePower, "W", 0),
    chargePercent,
    joinMeta([
      maxChargeCurrent !== null ? `Max ${formatReading(maxChargeCurrent, "A", 0)}` : null,
      formatReading(dischargeCurrent, "A", 1) ? `Dis ${formatReading(dischargeCurrent, "A", 1)}` : null,
    ]) || "Charge data unavailable"
  );
}

function createOverviewRow(title, entries) {
  const row = document.createElement("div");
  row.className = "overview-row";

  const label = document.createElement("div");
  label.className = "overview-label";
  label.textContent = title;
  row.appendChild(label);

  const values = document.createElement("div");
  values.className = "overview-values";

  entries.forEach((entry) => {
    const chip = document.createElement("div");
    chip.className = "value-chip";

    const key = document.createElement("small");
    key.textContent = entry.label;

    const value = document.createElement("strong");
    value.textContent = entry.value;

    chip.appendChild(key);
    chip.appendChild(value);
    values.appendChild(chip);
  });

  row.appendChild(values);
  return row;
}

function renderOverview(data) {
  renderMeters(data);

  const container = byId("overviewRows");
  if (!container) {
    return;
  }

  const fragment = document.createDocumentFragment();
  let visibleGroups = 0;

  OVERVIEW_GROUPS.forEach((group) => {
    const entries = group.fields
      .map((field) => {
        const value = pickDataValue(data, field.keys, field.sections);
        if (!isDataValuePresent(value)) {
          return null;
        }

        const display =
          field.numeric === false
            ? String(value).trim()
            : formatReading(value, field.unit || "", field.decimals ?? 0);

        return display ? { label: field.label, value: display } : null;
      })
      .filter(Boolean);

    if (!entries.length) {
      return;
    }

    fragment.appendChild(createOverviewRow(group.title, entries));
    visibleGroups += 1;
  });

  if (!visibleGroups) {
    const empty = document.createElement("div");
    empty.className = "overview-empty";
    empty.textContent = "Waiting for inverter data.";
    container.replaceChildren(empty);
    return;
  }

  container.replaceChildren(fragment);
}

function renderStatus(data) {
  data = data || {};

  if (window.StatusBar && typeof window.StatusBar.applyStatus === "function") {
    window.StatusBar.applyStatus(data);
  }

  setText("deviceProtocol", data.protocol || "-");
  setText("devicename", data.deviceName || data.EspData?.Device_name || "Solar2MQTT");
  setText("pageTitleDevice", data.EspData?.Device_name || "Solar2MQTT");
  setText("loopbackInfo", data.loopback?.message || "-");
  setText("metricPvPower", formatValue(totalSolarPower(data), " W"));
  setText("metricBatteryPercent", formatValue(data.LiveData?.Battery_Percent, " %"));
  setText("metricBatteryVoltage", formatValue(data.LiveData?.Battery_Voltage ?? data.LiveData?.Positive_Battery_Voltage, " V"));
  if (data.RawData && Object.prototype.hasOwnProperty.call(data.RawData, "CommandAnswer")) {
    setText("commandAnswer", data.RawData.CommandAnswer || "-");
  }

  renderOverview(data);
}

async function request(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return response;
}

async function fetchJson(url, options = {}) {
  const response = await request(url, options);
  return response.json();
}

async function fetchText(url, options = {}) {
  const response = await request(url, options);
  return response.text();
}

async function postForm(url, form) {
  const body = new URLSearchParams();

  Array.from(form.elements).forEach((field) => {
    if (!field.name || field.disabled) {
      return;
    }

    const type = (field.type || "").toLowerCase();

    if (type === "submit" || type === "button" || type === "reset" || type === "file") {
      return;
    }

    if (type === "checkbox") {
      body.set(field.name, field.checked ? (field.value || "1") : "0");
      return;
    }

    if (type === "radio") {
      if (field.checked) {
        body.set(field.name, field.value);
      }
      return;
    }

    body.set(field.name, field.value ?? "");
  });

  return fetchJson(url, { method: "POST", body });
}

async function download(url, filename, type) {
  const response = await request(url);
  const blob = await response.blob();
  const typedBlob = type ? new Blob([blob], { type }) : blob;
  const anchor = document.createElement("a");
  anchor.href = URL.createObjectURL(typedBlob);
  anchor.download = filename;
  anchor.click();
  URL.revokeObjectURL(anchor.href);
}

async function loadSettings() {
  if (!hasAny(["networkForm", "mqttForm", "deviceForm"])) {
    return;
  }

  const data = await fetchJson("/api/settings");
  fillForm("networkForm", data.network);
  fillForm("mqttForm", data.mqtt);
  fillForm("deviceForm", data.device);
}

async function loadStatus() {
  renderStatus(await fetchJson("/api/status"));
}

async function loadDataPreview() {
  if (!hasAny(["dataPreview", "commandAnswer"])) {
    return;
  }

  const data = (await fetchJson("/api/data")) || {};
  setText("commandAnswer", data.RawData?.CommandAnswer || "-");

  const preview = byId("dataPreview");
  if (preview) {
    preview.textContent = JSON.stringify(data, null, 2);
  }

  setText("dataPreviewMeta", `Last updated: ${new Date().toLocaleTimeString("en-GB")}`);
}

async function loadReportPreview() {
  const preview = byId("reportPreview");
  if (!preview) {
    return;
  }

  preview.textContent = await fetchText("/api/debug/report");
  setText("reportPreviewMeta", `Last updated: ${new Date().toLocaleTimeString("en-GB")}`);
}

function getCommandAnswerValue(data) {
  const answer = data?.RawData?.CommandAnswer;
  return typeof answer === "string" ? answer.trim() : "";
}

async function waitForCommandAnswer(timeoutMs = 3500, intervalMs = 150) {
  const startedAt = Date.now();
  let lastData = {};

  while ((Date.now() - startedAt) < timeoutMs) {
    lastData = (await fetchJson("/api/data")) || {};
    if (getCommandAnswerValue(lastData)) {
      return lastData;
    }
    await new Promise((resolve) => window.setTimeout(resolve, intervalMs));
  }

  return lastData;
}

function focusPreview(previewId) {
  const preview = byId(previewId);
  if (!preview) {
    return;
  }

  preview.classList.add("preview-focus");
  preview.scrollIntoView({ behavior: "smooth", block: "center" });

  if (state.previewFocusTimers[previewId]) {
    window.clearTimeout(state.previewFocusTimers[previewId]);
  }

  state.previewFocusTimers[previewId] = window.setTimeout(() => {
    preview.classList.remove("preview-focus");
    delete state.previewFocusTimers[previewId];
  }, 1400);
}

function connectStatusSocket() {
  const protocol = window.location.protocol === "https:" ? "wss" : "ws";
  state.statusSocket = new WebSocket(`${protocol}://${window.location.host}/ws-status`);
  state.statusSocket.onmessage = (event) => {
    renderStatus(JSON.parse(event.data));
  };
  state.statusSocket.onerror = () => {
    try {
      state.statusSocket.close();
    } catch (error) {
    }
  };
  state.statusSocket.onclose = () => {
    window.setTimeout(connectStatusSocket, 2000);
  };
}

function bindSubmit(formId, handler) {
  const form = byId(formId);
  if (!form) {
    return;
  }

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    clearNotice();

    try {
      await handler(event.currentTarget);
    } catch (error) {
      showNotice(error.message, true);
    }
  });
}

function bindClick(id, handler) {
  const button = byId(id);
  if (!button) {
    return;
  }

  button.addEventListener("click", async () => {
    clearNotice();

    try {
      await handler();
    } catch (error) {
      showNotice(error.message, true);
    }
  });
}

window.addEventListener("DOMContentLoaded", async () => {
  connectStatusSocket();
  await Promise.allSettled([loadStatus(), loadSettings()]);

  bindSubmit("networkForm", async (form) => {
    await postForm("/api/settings/network", form);
    showNotice("Network settings saved. Restart is being prepared.");
    window.setTimeout(() => {
      window.location.href = "/reboot";
    }, 500);
  });

  bindSubmit("mqttForm", async (form) => {
    await postForm("/api/settings/mqtt", form);
    showNotice("MQTT settings saved. Restart is being prepared.");
    window.setTimeout(() => {
      window.location.href = "/reboot";
    }, 500);
  });

  bindSubmit("deviceForm", async (form) => {
    await postForm("/api/settings/device", form);
    showNotice("UART configuration applied.");
  });

  bindSubmit("commandForm", async (form) => {
    await postForm("/api/command", form);
    const commandInput = byId("commandInput");
    if (commandInput) {
      commandInput.value = "";
    }
    const data = await waitForCommandAnswer();
    setText("commandAnswer", data.RawData?.CommandAnswer || "-");

    const preview = byId("dataPreview");
    if (preview) {
      preview.textContent = JSON.stringify(data || {}, null, 2);
    }
    setText("dataPreviewMeta", `Last updated: ${new Date().toLocaleTimeString("en-GB")}`);

    if (getCommandAnswerValue(data)) {
      showNotice("Command answer received.");
    } else {
      showNotice("Command sent. No answer received yet.", true);
    }
  });

  bindSubmit("otaForm", async () => {
    const fileInput = byId("firmwareFile");
    if (!fileInput || !fileInput.files.length) {
      throw new Error("Please select a firmware file first.");
    }

    const formData = new FormData();
    formData.append("update", fileInput.files[0]);
    await fetchJson("/update", { method: "POST", body: formData });
    showNotice("Firmware uploaded. Restart is being prepared.");
    window.setTimeout(() => {
      window.location.href = "/reboot";
    }, 500);
  });

  bindSubmit("restoreForm", async () => {
    const fileInput = byId("settingsRestoreFile");
    if (!fileInput || !fileInput.files.length) {
      throw new Error("Please select a settings file first.");
    }

    const content = await fileInput.files[0].text();
    await fetchJson("/api/settings/restore", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: content,
    });

    showNotice("Settings imported. Restart is being prepared.");
    window.setTimeout(() => {
      window.location.href = "/reboot";
    }, 500);
  });

  bindClick("loopbackBtn", async () => {
    await fetchJson("/api/loopback/start", { method: "POST" });
    showNotice("Loopback test started.");
  });

  bindClick("haBtn", async () => {
    await fetchJson("/api/mqtt/discovery", { method: "POST" });
    showNotice("HA discovery triggered.");
  });

  bindClick("backupBtn", async () => {
    await download("/api/settings/backup", "solar2mqtt-settings.json", "application/json");
  });

  bindClick("reportBtn", async () => {
    await download("/api/debug/report", "solar2mqtt-debug.txt", "text/plain");
  });

  bindClick("previewDataBtn", async () => {
    await loadDataPreview();
    focusPreview("dataPreview");
    showNotice("JSON data loaded.");
  });

  bindClick("previewReportBtn", async () => {
    await loadReportPreview();
    focusPreview("reportPreview");
    showNotice("Debug report loaded.");
  });

  bindClick("rebootBtn", async () => {
    await fetchJson("/api/reboot", { method: "POST" });
    window.location.href = "/reboot";
  });

  bindClick("rebootNowBtn", async () => {
    await fetchJson("/api/reboot", { method: "POST" });
    showNotice("Reboot triggered.");
  });

  bindClick("resetBtn", async () => {
    if (!window.confirm("Trigger factory reset now?")) {
      return;
    }

    await fetchJson("/api/factory-reset", { method: "POST" });
    showNotice("Factory reset triggered.");
  });

  bindClick("factoryResetNowBtn", async () => {
    await fetchJson("/api/factory-reset", { method: "POST" });
    showNotice("Factory reset triggered.");
  });

  await Promise.allSettled([loadDataPreview(), loadReportPreview()]);
});
