'use strict';
const assert = require('node:assert/strict');
const http = require('node:http');
const { capture, prepare, restore, validateSnapshot } = require('../../tools/battery_energy_migration.cjs');

async function test() {
  const deviceId = 'solar2mqtt_AABBCCDDEEFF';
  let posts = 0, reads = 0, settingsWrites = 0, boot = 'boot-A', failJob = false;
  let corrupt = false, uptime = 10000, rebootDuringCapture = false;
  let receipt = '', pending = true, job = {}, posted;
  const paths = [];
  const diagnostic = () => ({
    schema: 1, deviceId, bootId: boot, mqttTopic: 'solar/test-inverter-a', storage: { available: true },
    migration: { pending, receipt, recoveryRequired: corrupt }, job,
    chargeKwh: .75, dischargeKwh: 3.25,
  });
  const server = http.createServer(async (req, res) => {
    paths.push(req.method + ' ' + req.url);
    assert.equal(req.headers.authorization, 'Basic ' + Buffer.from('reader:secret-test').toString('base64'));
    res.setHeader('Content-Type', 'application/json');
    if (req.url === '/api/ping') {
      uptime = rebootDuringCapture ? Math.max(0, uptime - 1000) : uptime + 100;
      return res.end(JSON.stringify({ uptimeMs: uptime }));
    }
    if (req.url === '/api/status') return res.end(JSON.stringify({ hostName: 'solar-24', fw: 'old', protocol: 'PI30_MAX' }));
    if (req.url === '/api/settings') {
      if (req.method !== 'GET') ++settingsWrites;
      return res.end(JSON.stringify({ mqtt: { topic: 'solar/test-inverter-a', password: 'never-save-me' }, network: { wifiPassword0: 'also-secret' } }));
    }
    if (req.url === '/api/data') return res.end(JSON.stringify({ LiveData: { Battery_Charge_Energy: .5, Battery_Discharge_Energy: 2.5 } }));
    if (req.url === '/api/debug/energy/migration') {
      assert.equal(req.method, 'POST'); ++posts;
      let text = ''; for await (const chunk of req) text += chunk;
      posted = JSON.parse(text);
      assert.equal(posted.deviceId, deviceId); assert.equal(posted.bootId, boot);
      assert.equal(posted.historyMode, 'carry_visible_baseline'); assert.equal(posted.unit, 'kWh');
      assert.equal(posted.chargeKwh, .5); assert.equal(posted.dischargeKwh, 2.5);
      if (!pending && receipt === posted.requestId)
        return res.end(JSON.stringify({ success: true, state: 'already_applied', requestId: receipt }));
      job = { state: 'queued', requestId: posted.requestId };
      res.statusCode = 202;
      return res.end(JSON.stringify({ success: true, state: 'queued', requestId: posted.requestId }));
    }
    if (req.url === '/api/debug/energy') {
      ++reads;
      if (job.state === 'queued') {
        if (failJob) job = { ...job, state: 'failed', error: 'migration_checkpoint_not_verified' };
        else { receipt = job.requestId; pending = false; job.state = 'applied'; }
      }
      return res.end(JSON.stringify(diagnostic()));
    }
    res.statusCode = 404; res.end('{}');
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const url = `http://127.0.0.1:${server.address().port}`;
  const options = { user: 'reader', password: 'secret-test', pollTimeoutMs: 3000, pollIntervalMs: 1 };
  try {
    const snapshot = await capture(url, deviceId, options);
    assert.equal(snapshot.chargeKwh, .5); assert.equal(snapshot.dischargeKwh, 2.5);
    assert.equal(snapshot.identity.deviceId, deviceId);
    assert(!JSON.stringify(snapshot).includes('secret'));
    assert.equal(posts, 0); assert.equal(settingsWrites, 0);
    for (const key of ['unit', 'schema', 'requestId', 'historyMode', 'identity']) {
      const bad = structuredClone(snapshot); delete bad[key]; assert.throws(() => validateSnapshot(bad));
    }
    for (const value of [null, true, '1', -1, NaN, Infinity, 1e10]) {
      const bad = structuredClone(snapshot); bad.chargeKwh = value; assert.throws(() => validateSnapshot(bad));
    }
    for (const bad of [
      { ...diagnostic(), deviceId: 'solar2mqtt_112233445566' },
      { ...diagnostic(), mqttTopic: 'solar/test-inverter-b' },
      { ...diagnostic(), schema: 2 },
      { ...diagnostic(), storage: { available: false } },
      { ...diagnostic(), migration: { pending: false } },
    ]) assert.throws(() => prepare(snapshot, bad, { hostName: 'solar-24' }));
    assert.throws(() => prepare(snapshot, diagnostic(), { hostName: 'different-board' }));
    const payload = prepare(snapshot, diagnostic(), { hostName: 'solar-24' });
    assert.equal(payload.bootId, boot); assert.equal(posts, 0);
    corrupt = true;
    assert.throws(() => prepare(snapshot, diagnostic(), { hostName: 'solar-24' }));
    assert(prepare(snapshot, diagnostic(), { hostName: 'solar-24' }, { acknowledgeRecovery: true }).acknowledgeRecovery);
    corrupt = false;
    const result = await restore(snapshot, url, options);
    assert.equal(result.state, 'applied'); assert.equal(posts, 1); assert(reads >= 2);
    boot = 'boot-B';
    assert.equal((await restore(snapshot, url, options)).state, 'already_applied');
    assert.equal(posts, 2); assert.equal(posted.requestId, snapshot.requestId);
    const different = { ...snapshot, requestId: 'f'.repeat(32) };
    await assert.rejects(restore(different, url, options), /already initialized/);
    assert.equal(posts, 2);
    receipt = ''; pending = true; job = {}; failJob = true;
    await assert.rejects(restore(snapshot, url, options), /retain and retry the same snapshot/);
    rebootDuringCapture = true;
    await assert.rejects(capture(url, deviceId, options), /restarted during capture/);
    assert.equal(settingsWrites, 0);
    assert(paths.every(path => !path.includes('update_firmware') && !path.includes('restart')));
    console.log('PASS: battery migration capture/prepare/restore HTTP contract, credential omission, device guards, reboot/idempotency and write failures.');
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
}
module.exports = { test };
if (require.main === module) test().catch(error => { console.error(error); process.exitCode = 1; });
