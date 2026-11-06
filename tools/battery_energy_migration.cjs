#!/usr/bin/env node
'use strict';

// This tool never uploads firmware, resets a device, changes settings or edits HA.
// Only the explicit "restore" and "start-zero" commands submit a migration.
const fs = require('node:fs/promises');
const crypto = require('node:crypto');

function origin(value) {
  const url = new URL(value);
  if (!['http:', 'https:'].includes(url.protocol) || url.username || url.password ||
      url.pathname !== '/' || url.search || url.hash) throw Error('Expected a device origin, e.g. http://192.0.2.25');
  return url.origin;
}
function counter(value) {
  if (typeof value !== 'number' || !Number.isFinite(value) || value < 0 || value > 1e9)
    throw Error('Expected a finite nonnegative kWh counter');
  return value;
}
function validateSnapshot(snapshot) {
  if (!snapshot || snapshot.schema !== 1 || snapshot.unit !== 'kWh' ||
      !['carry_visible_baseline', 'start_new_series'].includes(snapshot.historyMode) ||
      !/^[0-9a-f]{32}$/.test(snapshot.requestId) || /^0+$/.test(snapshot.requestId) ||
      !/^solar2mqtt_[0-9A-F]{12}$/.test(snapshot.identity?.deviceId) ||
      typeof snapshot.identity?.hostName !== 'string' || !snapshot.identity.hostName ||
      typeof snapshot.identity?.mqttTopic !== 'string' || !snapshot.identity.mqttTopic ||
      !Number.isFinite(Date.parse(snapshot.capturedAt))) throw Error('Invalid battery snapshot');
  origin(snapshot.origin);
  counter(snapshot.chargeKwh);
  counter(snapshot.dischargeKwh);
  if (snapshot.historyMode === 'start_new_series' &&
      (snapshot.chargeKwh !== 0 || snapshot.dischargeKwh !== 0))
    throw Error('A zero-start snapshot must have both baselines equal to zero');
  return snapshot;
}

function client(deviceOrigin, { user = process.env.SOLAR2MQTT_USER, password = process.env.SOLAR2MQTT_PASSWORD,
                                timeoutMs = 8000 } = {}) {
  const base = origin(deviceOrigin);
  return async function json(path, body) {
    const headers = { Accept: 'application/json' };
    if (user) headers.Authorization = 'Basic ' + Buffer.from(`${user}:${password || ''}`).toString('base64');
    if (body !== undefined) headers['Content-Type'] = 'application/json';
    const response = await fetch(base + path, {
      method: body === undefined ? 'GET' : 'POST', headers,
      body: body === undefined ? undefined : JSON.stringify(body),
      signal: AbortSignal.timeout(timeoutMs), redirect: 'error', cache: 'no-store',
    });
    let data;
    try { data = await response.json(); } catch { throw Error(`${path}: expected JSON (HTTP ${response.status})`); }
    if (!response.ok) throw Error(`${path}: HTTP ${response.status}: ${data.error || 'request failed'}`);
    return data;
  };
}

async function capture(deviceOrigin, deviceId, options) {
  if (!/^solar2mqtt_[0-9A-F]{12}$/.test(deviceId)) throw Error('Supply the existing Home Assistant MQTT device ID');
  const get = client(deviceOrigin, options);
  const before = await get('/api/ping');
  const status = await get('/api/status');
  // The existing settings endpoint includes credentials. Keep only the topic;
  // never print or save the full response or authentication headers.
  const settings = await get('/api/settings');
  const data = await get('/api/data');
  const after = await get('/api/ping');
  if (!Number.isSafeInteger(before.uptimeMs) || !Number.isSafeInteger(after.uptimeMs) ||
      after.uptimeMs < before.uptimeMs) throw Error('Device restarted during capture; capture again');
  if (status.simulation || data.EspData?.Simulation_Active) throw Error('Cannot migrate a simulated counter');
  const snapshot = {
    schema: 1, unit: 'kWh', historyMode: 'carry_visible_baseline',
    requestId: crypto.randomBytes(16).toString('hex'), capturedAt: new Date().toISOString(),
    origin: origin(deviceOrigin),
    identity: { deviceId, hostName: status.hostName, mqttTopic: settings.mqtt?.topic },
    chargeKwh: counter(data.LiveData?.Battery_Charge_Energy),
    dischargeKwh: counter(data.LiveData?.Battery_Discharge_Energy),
    observed: { firmware: status.fw || data.EspData?.sw_version, protocol: status.protocol,
                uptimeMs: after.uptimeMs },
  };
  return validateSnapshot(snapshot);
}

async function prepareZero(deviceOrigin, deviceId, options = {}) {
  if (!/^solar2mqtt_[0-9A-F]{12}$/.test(deviceId))
    throw Error('Supply the existing Home Assistant MQTT device ID');
  const get = client(deviceOrigin, options);
  const diagnostics = await get('/api/debug/energy');
  const status = await get('/api/status');
  const snapshot = {
    schema: 1, unit: 'kWh', historyMode: 'start_new_series',
    requestId: crypto.randomBytes(16).toString('hex'), capturedAt: new Date().toISOString(),
    origin: origin(deviceOrigin),
    identity: { deviceId, hostName: status.hostName, mqttTopic: diagnostics.mqttTopic },
    chargeKwh: 0, dischargeKwh: 0,
    observed: { firmware: status.fw || diagnostics.firmware, protocol: status.protocol,
                bootId: diagnostics.bootId, sessionChargeKwh: counter(diagnostics.chargeKwh),
                sessionDischargeKwh: counter(diagnostics.dischargeKwh) },
  };
  prepare(snapshot, diagnostics, status, options);
  const after = await get('/api/debug/energy');
  if (after.bootId !== diagnostics.bootId)
    throw Error('Device restarted during preparation; prepare again');
  prepare(snapshot, after, status, options);
  return snapshot;
}

function prepare(snapshot, diagnostics, status, { acknowledgeRecovery = false } = {}) {
  validateSnapshot(snapshot);
  if (diagnostics.schema !== 1 || !diagnostics.bootId ||
      diagnostics.deviceId !== snapshot.identity.deviceId ||
      diagnostics.mqttTopic !== snapshot.identity.mqttTopic ||
      status.hostName !== snapshot.identity.hostName)
    throw Error('Device identity or MQTT topic differs from the saved snapshot');
  if (status.simulation) throw Error('Cannot migrate a simulated counter');
  const alreadyApplied = diagnostics.migration?.pending === false && diagnostics.migration?.receipt === snapshot.requestId;
  if (!alreadyApplied) {
    if (!diagnostics.storage?.available) throw Error('Battery storage is unavailable');
    if (!diagnostics.migration?.pending) throw Error('Counters already initialized; refusing an additional baseline');
    if (diagnostics.migration.recoveryRequired && !acknowledgeRecovery)
      throw Error('Corrupt checkpoints require --acknowledge-recovery');
    if (diagnostics.migration.writeUncertain && diagnostics.migration.uncertainRequest !== snapshot.requestId)
      throw Error('Retry the original migration snapshot after an uncertain write');
  }
  return {
    schema: 1, unit: 'kWh',
    operation: snapshot.historyMode === 'start_new_series' ? 'start_zero' : 'restore',
    historyMode: snapshot.historyMode,
    requestId: snapshot.requestId, deviceId: snapshot.identity.deviceId, bootId: diagnostics.bootId,
    chargeKwh: snapshot.chargeKwh, dischargeKwh: snapshot.dischargeKwh,
    acknowledgeRecovery,
  };
}

async function prepareFromDevice(snapshot, deviceOrigin, options = {}) {
  const get = client(deviceOrigin, options);
  const diagnostics = await get('/api/debug/energy');
  const status = await get('/api/status');
  return prepare(snapshot, diagnostics, status, options);
}

async function applyMigration(snapshot, deviceOrigin, options = {}) {
  const payload = await prepareFromDevice(snapshot, deviceOrigin, options);
  const request = client(deviceOrigin, options);
  const accepted = await request('/api/debug/energy/migration', payload);
  if (accepted.requestId !== payload.requestId || accepted.success !== true)
    throw Error('Unexpected migration acknowledgement');
  if (accepted.state === 'already_applied') return accepted;
  if (accepted.state !== 'queued') throw Error('Migration was not queued');
  const deadline = Date.now() + (options.pollTimeoutMs ?? 30000);
  while (Date.now() < deadline) {
    const state = await request('/api/debug/energy');
    if (state.deviceId !== payload.deviceId) throw Error('Device changed during migration');
    if (state.migration?.receipt === payload.requestId && state.migration.pending === false)
      return { success: true, state: 'applied', requestId: payload.requestId,
               chargeKwh: state.chargeKwh, dischargeKwh: state.dischargeKwh };
    if (state.bootId !== payload.bootId) throw Error('Device restarted; retry using the same snapshot');
    if (state.job?.requestId === payload.requestId && state.job.state === 'failed')
      throw Error(`Migration failed: ${state.job.error}; retain and retry the same snapshot`);
    await new Promise(resolve => setTimeout(resolve, options.pollIntervalMs ?? 250));
  }
  throw Error('Migration status timed out; retain and retry the same snapshot');
}

async function restore(snapshot, deviceOrigin, options = {}) {
  validateSnapshot(snapshot);
  if (snapshot.historyMode !== 'carry_visible_baseline')
    throw Error('Use start-zero to apply a zero-start snapshot');
  return applyMigration(snapshot, deviceOrigin, options);
}

async function startZero(snapshot, deviceOrigin, options = {}) {
  validateSnapshot(snapshot);
  if (snapshot.historyMode !== 'start_new_series')
    throw Error('Use restore to apply a captured baseline');
  return applyMigration(snapshot, deviceOrigin, options);
}

async function main(args) {
  const [command, ...rest] = args;
  const acceptedFlags = {
    capture: ['--url', '--device-id', '--out'],
    prepare: ['--url', '--snapshot', '--out', '--acknowledge-recovery'],
    restore: ['--url', '--snapshot', '--acknowledge-recovery'],
    'prepare-zero': ['--url', '--device-id', '--out', '--acknowledge-recovery'],
    'start-zero': ['--url', '--snapshot', '--acknowledge-recovery'],
  };
  if (!Object.hasOwn(acceptedFlags, command))
    throw Error('Use capture, prepare, restore, prepare-zero or start-zero; see Docs/battery-energy.md');
  const flags = {};
  for (let i = 0; i < rest.length; ++i) {
    const key = rest[i];
    if (!acceptedFlags[command].includes(key) || Object.hasOwn(flags, key))
      throw Error(`Unknown or repeated argument: ${key}`);
    flags[key] = key === '--acknowledge-recovery' ? true : rest[++i];
    if (!flags[key] || (typeof flags[key] === 'string' && flags[key].startsWith('--'))) throw Error(`Missing value: ${key}`);
  }
  const write = async value => {
    if (!flags['--out']) throw Error('Supply --out; files are created without overwriting an earlier capture');
    await fs.writeFile(flags['--out'], JSON.stringify(value, null, 2) + '\n', { flag: 'wx' });
    console.log(`Saved ${flags['--out']}`);
  };
  const options = { acknowledgeRecovery: !!flags['--acknowledge-recovery'] };
  if (command === 'capture') return write(await capture(flags['--url'], flags['--device-id']));
  if (command === 'prepare-zero')
    return write(await prepareZero(flags['--url'], flags['--device-id'], options));
  if (!flags['--snapshot']) throw Error('Supply --snapshot FILE');
  const snapshot = validateSnapshot(JSON.parse(await fs.readFile(flags['--snapshot'], 'utf8')));
  const url = flags['--url'] || snapshot.origin;
  if (command === 'prepare') return write(await prepareFromDevice(snapshot, url, options));
  const apply = command === 'start-zero' ? startZero : restore;
  console.log(JSON.stringify(await apply(snapshot, url, options), null, 2));
}

module.exports = { origin, counter, client, validateSnapshot, capture, prepareZero,
                   prepare, prepareFromDevice, restore, startZero, main };
if (require.main === module) main(process.argv.slice(2)).catch(error => {
  console.error(error.message);
  process.exitCode = 1;
});
