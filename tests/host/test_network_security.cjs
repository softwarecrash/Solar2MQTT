const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const root = path.join(__dirname, '../..');
const wifi = fs.readFileSync(path.join(root, 'src/webUI/wifi.html'), 'utf8');
const app = fs.readFileSync(path.join(root, 'src/webUI/app.js'), 'utf8');
new vm.Script(app);
for (const match of wifi.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g)) {
  new vm.Script(match[1]);
}

function wifiFunction(name) {
  const start = wifi.indexOf(`    function ${name}(`);
  assert(start >= 0);
  return wifi.slice(start, wifi.indexOf('\n    }', start) + 6);
}

const nodes = {};
const document = { getElementById: id => nodes[id] ||= { value: '', checked: false, dataset: {} } };
const context = { document, encodeURIComponent };
vm.createContext(context);
vm.runInContext(wifiFunction('setPasswordState') + '\n' + wifiFunction('buildSubmitBody'), context);
for (const id of ['password0', 'password1', 'webPass', 'apPassword']) {
  context.setPasswordState(id, true);
  assert.equal(nodes[id].value, '');
  assert.equal(nodes[id].dataset.passwordSet, 'true');
  assert.equal(nodes[id + 'Clear'].checked, false);
}

// Saving unrelated settings sends blank secrets without requesting deletion.
nodes.devicename = { value: 'New device name' };
let params = new URLSearchParams(context.buildSubmitBody());
for (const id of ['password0', 'password1', 'webPass', 'apPassword']) {
  assert.equal(params.get(id), '');
  assert.equal(params.get(id + 'Clear'), '0');
}

// Replacement passwords survive URL encoding; deletion is always explicit.
nodes.apPassword.value = 'a+b&c=d?123';
nodes.password1Clear.checked = true;
params = new URLSearchParams(context.buildSubmitBody());
assert.equal(params.get('apPassword'), 'a+b&c=d?123');
assert.equal(params.get('password1Clear'), '1');
assert.equal(params.get('password0Clear'), '0');
context.setPasswordState('apPassword', false);
assert.equal(nodes.apPassword.value, '');
assert.equal(nodes.apPassword.dataset.passwordSet, 'false');

// Guard against restoring the original AP authentication bypass or secret leak.
const server = fs.readFileSync(path.join(root, 'src/core/WebServerHandler.cpp'), 'utf8');
assert(!/isInApMode\(\)\s*&&\s*!isAuthorized/.test(server));
const auth = server.slice(server.indexOf('bool WebServerHandler::isAuthorized'), server.indexOf('void WebServerHandler::registerRoutes'));
assert(!auth.includes('isInApMode'));
assert(server.includes('_server.addMiddleware'));
assert(!/\["(?:wifiPassword[01]|webUIPassword|password|pass[01]|webPass)"\]\s*=\s*_settings\.get\./.test(server));
console.log('Network settings: password preservation, replacement, explicit deletion and security guards passed');

// Exercise the shared MQTT form serializer, including unchecked checkboxes.
const postStart = app.indexOf('async function postForm(');
const postSource = app.slice(postStart, app.indexOf('\n}', postStart) + 2);
const mqttContext = { URLSearchParams, fetchJson: async (url, options) => options.body };
vm.createContext(mqttContext);
vm.runInContext(postSource, mqttContext);
async function checkMqttForm() {
  const password = { name: 'password', type: 'password', value: '' };
  const clear = { name: 'passwordClear', type: 'checkbox', value: '1', checked: false };
  const form = { elements: [password, clear] };
  let body = await mqttContext.postForm('/api/settings/mqtt', form);
  assert.equal(body.get('password'), '');
  assert.equal(body.get('passwordClear'), '0');
  password.value = 'replacement&password';
  body = await mqttContext.postForm('/api/settings/mqtt', form);
  assert.equal(body.get('password'), 'replacement&password');
  clear.checked = true;
  body = await mqttContext.postForm('/api/settings/mqtt', form);
  assert.equal(body.get('passwordClear'), '1');
  console.log('MQTT form: unchanged, replaced and explicitly cleared passwords passed');
}
checkMqttForm();
