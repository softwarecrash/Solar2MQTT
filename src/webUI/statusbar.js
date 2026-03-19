(function () {
  if (window.__Solar2MqttStatusBarLoaded) {
    return;
  }
  window.__Solar2MqttStatusBarLoaded = true;

  function cssVar(name, fallback) {
    const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
    return value || fallback;
  }

  function safe(text) {
    return String(text || "").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  function iconWifi(color, level, title) {
    const opacity = (step) => (step > level ? "0.18" : "1");
    return `
      <svg viewBox="5 15 50 50" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="WiFi">
        <title>${safe(title)}</title>
        <path d="M8 32 A33 32 0 0 1 56 32" stroke="${color}" stroke-width="4" fill="none" opacity="${opacity(4)}"/>
        <path d="M14 38 A26 26 0 0 1 50 38" stroke="${color}" stroke-width="4" fill="none" opacity="${opacity(3)}"/>
        <path d="M20 44 A18 20 0 0 1 44 44" stroke="${color}" stroke-width="4" fill="none" opacity="${opacity(2)}"/>
        <path d="M26 50 A10 14 0 0 1 38 50" stroke="${color}" stroke-width="4" fill="none" opacity="${opacity(1)}"/>
        <circle cx="32" cy="56" r="3" fill="${color}"/>
      </svg>`;
  }

  function iconAp(color, title) {
    return `
      <svg viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="AP Mode">
        <title>${safe(title)}</title>
        <circle cx="32" cy="32" r="7" fill="${color}"/>
        <path d="M32 13c9 0 17.3 3.5 23.6 9.1" stroke="${color}" stroke-width="4" fill="none" stroke-linecap="round"/>
        <path d="M32 13c-9 0-17.3 3.5-23.6 9.1" stroke="${color}" stroke-width="4" fill="none" stroke-linecap="round"/>
        <path d="M32 21c6.2 0 11.9 2.4 16.2 6.4" stroke="${color}" stroke-width="4" fill="none" stroke-linecap="round"/>
        <path d="M32 21c-6.2 0-11.9 2.4-16.2 6.4" stroke="${color}" stroke-width="4" fill="none" stroke-linecap="round"/>
      </svg>`;
  }

  function iconMqtt(color, title) {
    return `
      <svg viewBox="0 0 320 320" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="MQTT">
        <title>${safe(title)}</title>
        <path fill="${color}" d="M7.1,180.6v117.1c0,8.4,6.8,15.3,15.3,15.3H142C141,239.8,80.9,180.7,7.1,180.6z"/>
        <path fill="${color}" d="M7.1,84.1v49.8c99,0.9,179.4,80.7,180.4,179.1h51.7C238.2,186.6,134.5,84.2,7.1,84.1z"/>
        <path fill="${color}" d="M312.9,297.6V193.5C278.1,107.2,207.3,38.9,119,7.1H22.4c-8.4,0-15.3,6.8-15.3,15.3v15
          c152.6,0.9,276.6,124,277.6,275.6h13C306.1,312.9,312.9,306.1,312.9,297.6z"/>
        <path fill="${color}" d="M272.6,49.8c14.5,14.4,28.6,31.7,40.4,47.8V22.4c0-8.4-6.8-15.3-15.3-15.3h-77.3
          C238.4,19.7,256.6,33.9,272.6,49.8z"/>
      </svg>`;
  }

  function iconInverter(color, title) {
    return `
      <svg viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Inverter">
        <title>${safe(title)}</title>
        <rect x="10" y="10" width="44" height="44" rx="8" fill="none" stroke="${color}" stroke-width="4"/>
        <path d="M22 24h20M22 32h20M22 40h12" stroke="${color}" stroke-width="4" stroke-linecap="round"/>
        <circle cx="45" cy="40" r="4" fill="${color}"/>
      </svg>`;
  }

  function iconService(color, title) {
    return `
      <svg viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Service">
        <title>${safe(title)}</title>
        <path d="M37.8 11.4a18.4 18.4 0 0 0-11.6 0l-2.2 6.5-6 2.9-6.3-2.5a18.8 18.8 0 0 0-5.8 10l5.2 4.7v7l-5.2 4.7a18.8 18.8 0 0 0 5.8 10l6.3-2.5 6 2.9 2.2 6.5a18.4 18.4 0 0 0 11.6 0l2.2-6.5 6-2.9 6.3 2.5a18.8 18.8 0 0 0 5.8-10l-5.2-4.7v-7l5.2-4.7a18.8 18.8 0 0 0-5.8-10l-6.3 2.5-6-2.9-2.2-6.5Z" fill="none" stroke="${color}" stroke-width="3" stroke-linejoin="round"/>
        <circle cx="32" cy="36" r="8" fill="none" stroke="${color}" stroke-width="4"/>
      </svg>`;
  }

  function wifiBars(rssi) {
    if (typeof rssi !== "number") {
      return 0;
    }
    if (rssi >= -55) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -72) return 2;
    if (rssi >= -82) return 1;
    return 0;
  }

  function view() {
    return `
      <div class="panel statusbar" role="group" aria-label="Status">
        <div class="status-item"><a class="status-link" href="/wifisetup"><div class="indicator" data-role="wifi"></div></a></div>
        <div class="status-item"><a class="status-link" href="/mqttsettings"><div class="indicator" data-role="mqtt"></div></a></div>
        <div class="status-item"><a class="status-link" href="/devicesettings"><div class="indicator" data-role="inverter"></div></a></div>
        <div class="status-item"><a class="status-link" href="/firmwareupdate"><div class="indicator" data-role="service"></div></a></div>
        <div class="status-version-overlay"><span data-role="version"></span></div>
      </div>`;
  }

  let root = null;
  let mounted = false;
  let socket = null;
  let reconnectTimer = null;
  let retry = 0;

  function okColor() {
    return cssVar("--ok", "#21c065");
  }

  function errColor() {
    return cssVar("--err", "#ff4b4b");
  }

  function warnColor() {
    return cssVar("--ea-primary", "#f6c549");
  }

  function ensureMounted() {
    if (!mounted) {
      mount("#statusbar");
    }
  }

  function mount(selector) {
    const host = document.querySelector(selector || "#statusbar");
    if (!host) {
      return null;
    }

    const wrap = document.createElement("div");
    wrap.innerHTML = view();
    host.replaceChildren(wrap.firstElementChild);
    root = host.firstElementChild;
    mounted = true;

    setWiFi(null, false);
    setMqtt(false);
    setInverter(false);
    setService(false);
    setVersion("");
    return root;
  }

  function setIndicator(role, html, extraClass, title) {
    ensureMounted();
    if (!root) {
      return;
    }

    const node = root.querySelector(`[data-role="${role}"]`);
    if (!node) {
      return;
    }

    node.innerHTML = html;
    node.classList.toggle("ok", extraClass === "ok");
    node.classList.toggle("err", extraClass === "err");
    node.classList.toggle("warn", extraClass === "warn");
    node.title = title || "";
    node.setAttribute("aria-label", title || role);
  }

  function setWiFi(rssi, apMode) {
    if (apMode) {
      setIndicator("wifi", iconAp(warnColor(), "AP mode active"), "warn", "AP mode active");
      return;
    }

    if (typeof rssi !== "number" || rssi === 0) {
      setIndicator("wifi", iconWifi(errColor(), 0, "WiFi offline"), "err", "WiFi offline");
      return;
    }

    const bars = wifiBars(rssi);
    const title = `RSSI ${rssi} dBm`;
    const color = bars > 0 ? okColor() : errColor();
    setIndicator("wifi", iconWifi(color, bars, title), bars > 0 ? "ok" : "err", title);
  }

  function setMqtt(connected) {
    setIndicator(
      "mqtt",
      iconMqtt(connected ? okColor() : errColor(), connected ? "MQTT connected" : "MQTT offline"),
      connected ? "ok" : "err",
      connected ? "MQTT connected" : "MQTT offline"
    );
  }

  function setInverter(connected) {
    setIndicator(
      "inverter",
      iconInverter(connected ? okColor() : errColor(), connected ? "Inverter connected" : "Inverter offline"),
      connected ? "ok" : "err",
      connected ? "Inverter connected" : "Inverter offline"
    );
  }

  function setService(apMode) {
    let color = okColor();
    let klass = "ok";
    let title = "Service ready";

    if (apMode) {
      color = warnColor();
      klass = "warn";
      title = "AP mode active";
    }

    setIndicator("service", iconService(color, title), klass, title);
  }

  function setVersion(version) {
    ensureMounted();
    if (!root) {
      return;
    }

    const node = root.querySelector("[data-role=\"version\"]");
    if (node) {
      node.textContent = version ? `V${version}` : "";
    }
  }

  function applyStatus(message) {
    if (!message || typeof message !== "object") {
      return;
    }

    const wifiData = message.wifi && typeof message.wifi === "object" ? message.wifi : {};
    const rssi =
      typeof wifiData.rssi === "number"
        ? wifiData.rssi
        : (message.EspData && typeof message.EspData.Wifi_RSSI === "number" ? message.EspData.Wifi_RSSI : null);
    const apMode = typeof wifiData.apMode === "boolean" ? wifiData.apMode : message.apMode === true;

    setWiFi(rssi, apMode);
    setMqtt(message.mqtt === true || message.mqttConnected === true);
    setInverter(message.inverter === true || message.inverterConnected === true);
    setService(apMode);
    setVersion(message.fw || (message.EspData && message.EspData.sw_version) || "");
  }

  function scheduleReconnect() {
    if (socket) {
      try {
        socket.close();
      } catch (error) {
      }
    }
    socket = null;

    const delay = Math.min(5000, 500 + retry * 500);
    retry = Math.min(retry + 1, 10);

    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
    }
    reconnectTimer = window.setTimeout(connect, delay);
  }

  function connect() {
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    try {
      socket = new WebSocket(`${protocol}://${window.location.host}/ws-status`);
    } catch (error) {
      scheduleReconnect();
      return;
    }

    socket.addEventListener("open", () => {
      retry = 0;
      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
      }
    });

    socket.addEventListener("message", (event) => {
      try {
        applyStatus(JSON.parse(event.data));
      } catch (error) {
      }
    });

    socket.addEventListener("close", scheduleReconnect);
    socket.addEventListener("error", scheduleReconnect);
  }

  window.StatusBar = {
    mount,
    applyStatus,
    setWiFi,
    setMqtt,
    setInverter,
    setService,
    setVersion,
  };

  document.addEventListener("DOMContentLoaded", () => {
    if (document.getElementById("statusbar")) {
      mount("#statusbar");
    }
    connect();
  });
})();
