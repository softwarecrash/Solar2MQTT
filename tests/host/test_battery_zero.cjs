'use strict';
const assert = require('node:assert/strict');
const http = require('node:http');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const tool = require('../../tools/battery_energy_migration.cjs');

const deviceId = 'solar2mqtt_AABBCCDDEEFF';
const auth = { user: 'reader', password: 'secret-test', pollTimeoutMs: 3000, pollIntervalMs: 1 };

async function fixture() {
  let state;
  const reset = () => {
    state = {
      boot: 'boot-A', pending: true, receipt: '', job: {}, chargeKwh: .75, dischargeKwh: 3.25,
      corrupt: false, available: true, simulation: false, uncertainRequest: '',
      posts: 0, applications: 0, paths: [],
    };
  };
  reset();
  const diagnostics = () => ({
    schema: 1, deviceId, bootId: state.boot, mqttTopic: 'solar/example', firmware: 'test',
    storage: { available: state.available },
    migration: { pending: state.pending, receipt: state.receipt, recoveryRequired: state.corrupt,
                 writeUncertain: !!state.uncertainRequest, uncertainRequest: state.uncertainRequest },
    job: state.job, chargeKwh: state.chargeKwh, dischargeKwh: state.dischargeKwh,
  });
  const apply = () => {
    ++state.applications;
    state.chargeKwh += state.posted.chargeKwh;
    state.dischargeKwh += state.posted.dischargeKwh;
    state.receipt = state.posted.requestId;
    state.pending = false;
    state.job.state = 'applied';
  };
  const server = http.createServer(async (req, res) => {
    state.paths.push(req.method + ' ' + req.url);
    res.setHeader('Content-Type', 'application/json');
    const reply = value => res.end(JSON.stringify(value));
    try {
      assert.equal(req.headers.authorization, 'Basic ' + Buffer.from('reader:secret-test').toString('base64'));
      if (req.url === '/api/ping') return reply({ uptimeMs: 10000 });
      if (req.url === '/api/status')
        return reply({ hostName: 'solar-example', fw: 'test', protocol: 'PI30_MAX', simulation: state.simulation });
      if (req.url === '/api/settings') {
        assert.equal(req.method, 'GET');
        return reply({ mqtt: { topic: 'solar/example', password: 'never-save-me' } });
      }
      if (req.url === '/api/data') return reply({ LiveData: {} });
      if (req.url === '/api/debug/energy/migration') {
        assert.equal(req.method, 'POST');
        ++state.posts;
        let text = '';
        for await (const chunk of req) text += chunk;
        state.posted = JSON.parse(text);
        assert.equal(state.posted.deviceId, deviceId);
        assert.equal(state.posted.bootId, state.boot);
        assert.equal(state.posted.operation, 'start_zero');
        assert.equal(state.posted.historyMode, 'start_new_series');
        assert.equal(state.posted.unit, 'kWh');
        assert.equal(state.posted.chargeKwh, 0);
        assert.equal(state.posted.dischargeKwh, 0);
        if (!state.pending && state.receipt === state.posted.requestId)
          return reply({ success: true, state: 'already_applied', requestId: state.receipt });
        state.job = { state: 'queued', requestId: state.posted.requestId };
        if (state.loseAcknowledgement) {
          state.loseAcknowledgement = false;
          apply();
          res.statusCode = 500;
          return reply({ error: 'simulated_lost_acknowledgement' });
        }
        res.statusCode = 202;
        return reply({ success: true, state: 'queued', requestId: state.posted.requestId });
      }
      if (req.url === '/api/debug/energy') {
        if (state.rebootDuringPreparation) state.boot = state.boot === 'boot-A' ? 'boot-B' : 'boot-A';
        if (state.job.state === 'queued') {
          if (state.rebootDuringApply) {
            state.rebootDuringApply = false;
            state.boot = 'boot-after-restart';
            state.job = {};
          } else if (state.failJob)
            state.job = { ...state.job, state: 'failed', error: 'migration_checkpoint_not_verified' };
          else apply();
        }
        return reply(diagnostics());
      }
      res.statusCode = 404;
      return reply({ error: 'unexpected_path' });
    } catch (error) {
      state.serverFailure = error;
      res.statusCode = 500;
      return reply({ error: 'test_contract_failure' });
    }
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  return { url: `http://127.0.0.1:${server.address().port}`, reset,
    get state() { return state; }, close: () => new Promise(resolve => server.close(resolve)) };
}

async function test() {
  const f = await fixture();
  const tempRoot = path.resolve(os.tmpdir());
  const directory = await fs.mkdtemp(path.join(tempRoot, 'solar2mqtt-zero-tests-'));
  const previousUser = process.env.SOLAR2MQTT_USER, previousPassword = process.env.SOLAR2MQTT_PASSWORD;
  try {
    await assert.rejects(tool.capture(f.url, deviceId, auth), /finite nonnegative/);
    f.state.paths = [];
    const zero = await tool.prepareZero(f.url, deviceId, auth);
    assert.equal(zero.historyMode, 'start_new_series');
    assert.equal(zero.chargeKwh, 0);
    assert.equal(zero.dischargeKwh, 0);
    assert.equal(zero.observed.sessionChargeKwh, .75);
    assert.equal(f.state.posts, 0);
    assert(!JSON.stringify(zero).includes('secret'));
    assert(!f.state.paths.some(route => route.includes('/api/settings')));
    for (const key of ['chargeKwh', 'dischargeKwh'])
      assert.throws(() => tool.validateSnapshot({ ...zero, [key]: .01 }), /equal to zero/);
    await assert.rejects(tool.restore(zero, f.url, auth), /Use start-zero/);
    await assert.rejects(tool.startZero({ ...zero, historyMode: 'carry_visible_baseline' }, f.url, auth), /Use restore/);
    assert.equal(f.state.posts, 0);
    assert.equal((await tool.startZero(zero, f.url, auth)).state, 'applied');
    assert.equal(f.state.chargeKwh, .75);
    assert.equal(f.state.dischargeKwh, 3.25);
    f.state.boot = 'boot-B';
    assert.equal((await tool.startZero(zero, f.url, auth)).state, 'already_applied');
    assert.equal(f.state.applications, 1);
    await assert.rejects(tool.prepareZero(f.url, deviceId, auth), /already initialized/);
    console.log('PASS: zero-start needs no old counters, preserves a positive pending session and never resets initialized totals.');

    for (const failure of ['available', 'simulation', 'corrupt', 'rebootDuringPreparation', 'uncertainRequest']) {
      f.reset();
      f.state[failure] = failure === 'available' ? false : failure === 'uncertainRequest' ? 'f'.repeat(32) : true;
      await assert.rejects(tool.prepareZero(f.url, deviceId, auth));
      assert.equal(f.state.posts, 0);
    }
    f.reset();
    await assert.rejects(tool.prepareZero(f.url, 'solar2mqtt_112233445566', auth), /identity/);
    await assert.rejects(tool.prepareZero(f.url, 'not-an-id', auth), /device ID/);
    f.state.corrupt = true;
    const recovered = await tool.prepareZero(f.url, deviceId, { ...auth, acknowledgeRecovery: true });
    await assert.rejects(tool.startZero(recovered, f.url, auth), /acknowledge-recovery/);
    await tool.startZero(recovered, f.url, { ...auth, acknowledgeRecovery: true });

    for (const failure of ['loseAcknowledgement', 'rebootDuringApply', 'timeout']) {
      f.reset();
      f.state[failure] = true;
      const saved = await tool.prepareZero(f.url, deviceId, auth);
      const options = failure === 'timeout' ? { ...auth, pollTimeoutMs: 0 } : auth;
      await assert.rejects(tool.startZero(saved, f.url, options));
      await tool.startZero(saved, f.url, auth);
      assert.equal(f.state.applications, 1);
      assert.equal(f.state.posted.requestId, saved.requestId);
      assert.equal(f.state.chargeKwh, .75);
    }
    f.reset();
    f.state.failJob = true;
    const failed = await tool.prepareZero(f.url, deviceId, auth);
    await assert.rejects(tool.startZero(failed, f.url, auth), /retain and retry the same snapshot/);
    console.log('PASS: zero-start rejects unavailable/simulated/changed/corrupt/uncertain devices; lost acknowledgement, timeout and reboot reuse the receipt.');

    f.reset();
    process.env.SOLAR2MQTT_USER = auth.user;
    process.env.SOLAR2MQTT_PASSWORD = auth.password;
    const file = path.join(directory, 'zero.json');
    const prepareArgs = ['prepare-zero', '--url', f.url, '--device-id', deviceId, '--out', file];
    await tool.main(prepareArgs);
    const savedText = await fs.readFile(file, 'utf8'), saved = JSON.parse(savedText);
    assert.equal(f.state.posts, 0);
    assert(!savedText.includes('secret'));
    await assert.rejects(tool.main(prepareArgs), /EEXIST/);
    assert.equal(await fs.readFile(file, 'utf8'), savedText);
    await tool.main(['prepare', '--snapshot', file, '--out', path.join(directory, 'request.json')]);
    assert.equal(JSON.parse(await fs.readFile(path.join(directory, 'request.json'))).operation, 'start_zero');
    assert.equal(f.state.posts, 0);
    await tool.main(['start-zero', '--snapshot', file]);
    f.state.boot = 'boot-C';
    await tool.main(['start-zero', '--snapshot', file]);
    assert.equal(f.state.applications, 1);
    assert.equal(f.state.posted.requestId, saved.requestId);
    assert.equal(f.state.chargeKwh, .75);
    const posts = f.state.posts;
    for (const args of [[], ['unknown'], ['start-zero'], ['start-zero', '--out', file],
      ['prepare-zero', '--url', f.url, '--url', f.url], ['prepare-zero', '--out'],
      ['prepare-zero', '--acknowledge-recovery', '--acknowledge-recovery']])
      await assert.rejects(tool.main(args));
    assert.equal(f.state.posts, posts);
    assert(!f.state.paths.some(route => /update_firmware|restart|reboot|settings\//.test(route)));
    assert(!f.state.serverFailure);
    console.log('PASS: actual CLI prepare-zero/preview/start-zero, exclusive output, stable request file and strict arguments; loopback HTTP only.');
  } finally {
    if (previousUser === undefined) delete process.env.SOLAR2MQTT_USER;
    else process.env.SOLAR2MQTT_USER = previousUser;
    if (previousPassword === undefined) delete process.env.SOLAR2MQTT_PASSWORD;
    else process.env.SOLAR2MQTT_PASSWORD = previousPassword;
    await f.close();
    if (path.dirname(path.resolve(directory)) !== tempRoot || !path.basename(directory).startsWith('solar2mqtt-zero-tests-'))
      throw Error('Unexpected test directory');
    await fs.rm(directory, { recursive: true, force: true });
  }
}

module.exports = { test };
if (require.main === module) test().catch(error => { console.error(error); process.exitCode = 1; });
