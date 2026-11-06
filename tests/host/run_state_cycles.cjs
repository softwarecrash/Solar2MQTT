'use strict';
const { harness } = require('./support/native.cjs');
const { extract } = require('./support/extract.cjs');
const writeMocks = require('./support/mocks.cjs');
const production = require('./support/production.cjs');

const groups = ["pi"];

function runGroup(group, options = {}) {
  if (!groups.includes(group)) throw Error('Unknown native group: ' + group);
  const h = harness(group, options);
  try {
    writeMocks(h);
    const sources = ['src/core/SolarState.cpp'];
    let test;
    if (['pi', 'pi-energy', 'queries'].includes(group)) {
      production.pi(h, { queries: group === 'queries' });
      test = { pi: 'test_state_cycles.cpp', 'pi-energy': 'test_pi_energy.cpp', queries: 'test_custom_queries.cpp' }[group];
    } else if (group === 'energy') test = 'test_battery_energy.cpp';
    else if (group === 'storage') test = 'test_battery_storage.cpp';
    else if (group === 'battery-api') {
      production.mqtt(h, { api: true });
      test = 'test_battery_api.cpp';
    } else {
      const setup = require(group === 'modbus' ? './modbus_test_support.cjs' : './restart_test_support.cjs');
      const extra = setup({ ...h, extract });
      test = extra.test;
      sources.push(...extra.sources);
    }
    if (group !== 'pi') sources.push('src/core/EnergyBacklog.cpp');
    const executable = h.compile('tests/host/' + test, sources, { esp32Abi: true });
    h.run(executable, []);
  } finally {
    h.cleanup();
  }
}

function main(args = []) {
  const selected = args[0] || 'state';
  if (args.length > 1) throw Error('Supply one native test group');
  if (selected === '--list') return console.log(groups.join('\n'));
  if (selected === 'state') {
    for (const group of ["pi"]) runGroup(group);
    return;
  }
  runGroup(selected.replace(/^--/, ''));
}

module.exports = { groups, runGroup, main };
if (require.main === module) {
  try { main(process.argv.slice(2)); }
  catch (error) { console.error(error); process.exitCode = 1; }
}
