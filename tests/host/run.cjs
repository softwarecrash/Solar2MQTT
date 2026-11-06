'use strict';
const { harness } = require('./support/native.cjs');
const native = require('./run_state_cycles.cjs');

const portable = ["snapshot-memory","pi-transport","powmr-pv2","pv-total","qpigs2","pv2-polling","qpigs","harness"];
const groups = [...portable, ...native.groups];

async function runGroup(group) {
  if (native.groups.includes(group)) return native.runGroup(group);
  if (group === 'migration') return require('./test_battery_migration.cjs').test();
  if (group === 'zero-start') return require('./test_battery_zero.cjs').test();
  if (group === 'harness') return require('./test_harness.cjs').test();
  if (group === 'ota') return require('./run_github_ota.cjs').runTests();
  if (group === 'status-memory') return require('./run_status_memory.cjs').runTests();
  if (group === 'snapshot-memory') return require('./run_snapshot_memory.cjs').runTests();
  if (group === 'snapshot-serialization') return require('./run_snapshot_serialization.cjs').runTests();
  if (group === 'discovery-memory') return require('./run_discovery_memory.cjs').runTests();
  if (group === 'firmware-writer') return require('./writer/run.cjs').runTests();
  if (group === 'ota-ui') return require('./test_ota_ui.cjs').test();
  if (group === 'isolation') return require('./test_isolation.cjs').test();
  if (group === 'pv2-polling') return require('./run_pv2_polling.cjs').runTests();
  if (group === 'qpigs') return require('./run_qpigs.cjs').runTests();
  if (group === 'qpigs2') return require('./run_qpigs2.cjs').runTests();
  if (group === 'pv-total') {
    delete require.cache[require.resolve('./test_pv_total.cjs')];
    return require('./test_pv_total.cjs');
  }
  const file = { 'pi-transport': 'test_pi_serial.cpp', 'powmr-pv2': 'test_powmr_pv2.cpp' }[group];
  if (!file) throw Error('Unknown host test group: ' + group);
  const h = harness(group);
  try {
    h.run(h.compile('tests/host/' + file, [], { mocked: false }), []);
  } finally {
    h.cleanup();
  }
}

async function main(args = []) {
  if (args.length > 1 && ['--list', 'all', 'portable'].includes(args[0]))
    throw Error('Use ' + args[0] + ' alone');
  if (args[0] === '--list') return console.log(groups.join('\n'));
  const selected = args.length === 0 || args[0] === 'all' ? groups : args[0] === 'portable' ? portable : args;
  for (const group of selected) if (!groups.includes(group)) throw Error('Unknown host test group: ' + group);
  for (const group of selected) {
    console.log('\nRunning ' + group);
    await runGroup(group);
  }
  console.log('\nPassed ' + selected.length + ' host test groups.');
}

module.exports = { groups, portable, runGroup, main };
if (require.main === module) main(process.argv.slice(2)).catch(error => { console.error(error); process.exitCode = 1; });
