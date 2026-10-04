const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

const source = fs.readFileSync("src/webUI/inverter.js", "utf8");
const elements = new Map();
const handlers = new Map();
let domReady = null;
let commandPosts = 0;

function makeElement(id) {
  const element = {
    id,
    value: id === "invBatteryType" ? "AGM" : "",
    checked: false,
    disabled: false,
    textContent: "",
    dataset: {},
    style: {},
    addEventListener(type, fn) {
      handlers.set(`${id}:${type}`, fn);
    },
  };
  elements.set(id, element);
  return element;
}

const context = {
  console,
  URLSearchParams,
  document: {
    getElementById(id) {
      return elements.get(id) || makeElement(id);
    },
  },
  window: {
    addEventListener(type, fn) {
      if (type === "DOMContentLoaded") domReady = fn;
    },
    setTimeout,
  },
  fetch: async (url, options = {}) => {
    if (url === "/api/status") {
      return { ok: true, json: async () => ({ protocol: "MODBUS_POWMR_PI" }) };
    }
    if (url === "/api/data") {
      return {
        ok: true,
        json: async () => ({
          DeviceData: { Output_Source_Priority: "SBU priority" },
          RawData: { CommandAnswer: "" },
          LiveData: {},
        }),
      };
    }
    if (url === "/api/command" && options.method === "POST") {
      ++commandPosts;
      return { ok: true, json: async () => ({ ok: true }) };
    }
    throw new Error(`Unexpected fetch: ${url}`);
  },
};

(async () => {
  vm.createContext(context);
  vm.runInContext(source, context);
  assert.equal(typeof domReady, "function");
  await domReady();

const batteryType = elements.get("invBatteryType");
assert.equal(batteryType.value, "AGM");
assert.equal(batteryType.disabled, true, "missing Battery_Type must be disabled");

const submit = handlers.get("inverterSettingsForm:submit");
assert.equal(typeof submit, "function");
await submit({ preventDefault() {} });

assert.equal(commandPosts, 0, "Save without edits must not write an unavailable HTML default");
  assert.equal(elements.get("inverterSettingsResult").textContent, "No changes.");
})().catch((error) => {
  console.error(error);
  process.exit(1);
});
