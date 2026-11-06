'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { runGroup } = require('./run_state_cycles.cjs');
const { extract } = require('./support/extract.cjs');

function test() {
  const project = path.resolve(__dirname, '../..'), tempRoot = path.resolve(os.tmpdir());
  const root = fs.mkdtempSync(path.join(tempRoot, 'solar2mqtt-isolation-'));
  const priorJson = process.env.ARDUINOJSON_INCLUDE;
  const copy = relative => {
    const target = path.join(root, relative);
    fs.mkdirSync(path.dirname(target), { recursive: true });
    fs.cpSync(path.join(project, relative), target, { recursive: true });
  };
  try {
    for (const file of ['tests/host', 'src/core/EnergyBacklog.cpp', 'src/core/EnergyBacklog.h',
      'src/core/SolarState.cpp', 'src/core/SolarState.h', 'src/core/JsonOutput.h', 'src/solar/SolarTypes.h', 'src/descriptors.h', 'src/modbus']) copy(file);
    process.env.ARDUINOJSON_INCLUDE = priorJson || path.join(project, '.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
    assert(!fs.existsSync(path.join(root, 'src/PI_Serial')));
    assert(!fs.existsSync(path.join(root, 'src/core/MqttHandler.cpp')));
    runGroup('energy', { root });
    runGroup('storage', { root });
    runGroup('modbus', { root });

    copy('src/core/MqttHandler.h');
    copy('src/solar/HaDiscoveryCatalog.h');
    copy('src/core/MqttHandler.cpp');
    runGroup('battery-api', { root });
    const mqttPath = path.join(root, 'src/core/MqttHandler.cpp');
    const completeMqtt = fs.readFileSync(mqttPath, 'utf8');
    const required = extract(completeMqtt, 'int MqttHandler::queueBatteryMigration');
    fs.writeFileSync(mqttPath, completeMqtt.replace(required, ''));
    assert.throws(() => runGroup('battery-api', { root }), /Missing production function int MqttHandler::queueBatteryMigration/);

    // The restart group sees only the actual flush body, not migration or
    // discovery implementations. It must never read the PI driver.
    fs.writeFileSync(mqttPath, extract(completeMqtt, 'void MqttHandler::flushPersistentEnergy'));
    copy('src/core/GitHubOtaUpdater.cpp');
    copy('src/core/GitHubOtaUpdater.h');
    copy('src/core/FirmwareWriter.h');
    copy('src/main.cpp');
    runGroup('restart', { root });
    require('./run_github_ota.cjs').runTests([], {root});

    // Conversely, PI/publication/query tests need no battery API methods.
    copy('src/PI_Serial');
    copy('src/solar/SolarInverterService.cpp');
    copy('src/solar/SolarInverterService.h');
    const apiMethods = ['void MqttHandler::initializeBatteryApi', 'void MqttHandler::refreshBatteryDiagnostics',
      'void MqttHandler::copyBatteryDiagnostics', 'int MqttHandler::queueBatteryMigration',
      'void MqttHandler::processBatteryMigration', 'void MqttHandler::preserveBatteryDiscoveryTopics'];
    let withoutApi = completeMqtt;
    for (const signature of apiMethods) withoutApi = withoutApi.replace(extract(completeMqtt, signature), '');
    fs.writeFileSync(mqttPath, withoutApi);
    const piPath = path.join(root, 'src/PI_Serial/PI_Serial.cpp');
    const completePi = fs.readFileSync(piPath, 'utf8');
    let withoutQueue = completePi;
    for (const signature of ['PI_Serial::PI_Serial', 'PI_Serial::~PI_Serial', 'String PI_Serial::sendCommand',
      'bool PI_Serial::isReadOnlyPiQuery', 'bool PI_Serial::sendCustomCommand'])
      withoutQueue = withoutQueue.replace(extract(completePi, signature), '');
    fs.writeFileSync(piPath, withoutQueue);
    runGroup('pi', { root });
    fs.writeFileSync(piPath, completePi);
    runGroup('queries', { root });
    console.log('PASS: isolated groups build/run without unrelated PI, migration or query implementations; a missing required API function fails visibly.');
  } finally {
    if (priorJson === undefined) delete process.env.ARDUINOJSON_INCLUDE;
    else process.env.ARDUINOJSON_INCLUDE = priorJson;
    if (path.dirname(path.resolve(root)) !== tempRoot || !path.basename(root).startsWith('solar2mqtt-isolation-'))
      throw Error('Unexpected isolation directory');
    fs.rmSync(root, { recursive: true, force: true });
  }
}

module.exports = { test };
if (require.main === module) test();
