'use strict';
const {harness} = require('./support/native.cjs');

function runTests() {
  const h = harness('ha-discovery');
  try {
    require('./ota_support/mocks.cjs')(h);
    // The production reconnect, dispatch and callback paths run against a
    // recording MQTT transport. Only entity serialization is substituted.
    const source = h.read('src/core/MqttHandler.cpp');
    const signatures = [
      'const HaEntityDescriptor *findDescriptor(', 'bool containsTopic(',
      'bool isPowMrProtocolName(', 'bool isPowMrPiHybridProtocolName(',
      'bool isPowMrWritableSettingKey(', 'String buildDiscoveryTopic(',
      'void purgeHaDiscoveryKey(', 'void purgeHaDiscoveryComponent(',
      'bool MqttHandler::ensureConnected()', 'void MqttHandler::triggerHaDiscovery()',
      'void MqttHandler::publishHaDiscovery(bool force)',
      'void MqttHandler::loop()',
      'void MqttHandler::handleMessage(', 'void MqttHandler::startHaDiscoverySweep()',
      'void MqttHandler::stopHaDiscoverySweep()'
    ];
    h.write('ha_production.h', signatures.map(signature => {
      const start = source.indexOf(signature);
      const end = source.indexOf('\n}', start);
      if (start < 0 || end < 0) throw Error('Missing production function: ' + signature);
      return source.slice(start, end + 2);
    }).join('\n\n'));
    h.run(h.compile('tests/host/test_ha_discovery.cpp'), []);
  } finally { h.cleanup(); }
}
module.exports = {runTests};
if (require.main === module) runTests();
