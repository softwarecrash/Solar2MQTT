'use strict';
const { extract } = require('./extract.cjs');

const publication = [
  'String sanitizeRawMqttText', 'void publishJsonValue', 'void MqttHandler::publishState',
  'void MqttHandler::publishFlatState', 'void MqttHandler::publishJsonState',
  'void MqttHandler::publishObjectSection', 'void MqttHandler::publishRawState',
];
const discovery = [
  'bool stringEqualsAny', 'const HaEntityDescriptor *findDescriptor', 'bool containsTopic',
  'void appendTopicIfMissing', 'bool isPowMrProtocolName', 'bool isPowMrPiHybridProtocolName',
  'bool isPowMrParallelOnlyKey', 'bool isPowMrWritableSettingKey', 'bool isDiscoverableValue',
  'String buildDiscoveryTopic', 'void purgeHaDiscoveryKey', 'void purgeHaDiscoveryComponent',
  'String getHaDeviceId', 'String buildUniqueId', 'String getDiscoveryModel', 'void populateDeviceInfo',
  'void populateEqualizationDeviceInfo', 'bool isPowMrEqualizationKey',
  'void MqttHandler::publishHaDiscovery', 'void MqttHandler::publishHaSection',
  'bool MqttHandler::hasHaDiscoveryTopic', 'void MqttHandler::initializeBatteryApi',
  'void MqttHandler::refreshBatteryDiagnostics', 'void MqttHandler::copyBatteryDiagnostics',
  'int MqttHandler::queueBatteryMigration', 'void MqttHandler::processBatteryMigration',
  'void MqttHandler::preserveBatteryDiscoveryTopics',
];

function mqtt(harness, { api = false, fullDiscovery = false } = {}) {
  const source = harness.read('src/core/MqttHandler.cpp');
  const completeDiscovery = fullDiscovery ? ['void MqttHandler::publishHaEspInternalTemperature',
    'void MqttHandler::publishHaDs18b20', 'void MqttHandler::publishHaPowMrSettings'] : [];
  harness.write('mqtt_state_generated.h', [...publication, ...(api ? discovery : []), ...completeDiscovery]
    .map(signature => extract(source, signature)).join('\n'));
}

function pi(harness, { queries = false } = {}) {
  const source = harness.read('src/PI_Serial/PI_Serial.cpp');
  const methods = [
    'String sanitizePiReplyText',
    'void copyJsonObject', 'bool PI_Serial::loop', 'void PI_Serial::beginCycleBackup',
    'void PI_Serial::restoreCycleBackup', 'void PI_Serial::clearCycleBackup',
    'void PI_Serial::markSuccessfulDynamicCycle', 'unsigned long PI_Serial::connectionHoldMs',
    'bool PI_Serial::hasFreshBatterySample', 'BatterySampleTiming PI_Serial::batterySampleTiming',
    'void PI_Serial::noteBatterySample', 'void PI_Serial::noteSuccessfulReply',
    'void PI_Serial::notifyStateUpdate', 'bool PI_Serial::isSuspended', 'bool PI_Serial::isModbus',
    'char *PI_Serial::getModeDesc', 'int countDelimitedFields', 'bool hasFlagSymbol',
    'bool isNumericProtocolId', 'bool hasRevoSignature', 'bool PI_Serial::isValidResponse',
    'void PI_Serial::refineProtocol', 'bool PI_Serial::requestAndStoreRaw',
    'bool PI_Serial::requestUnsupportedPiStatic', 'bool PI_Serial::requestUnsupportedPiDynamic',
    'bool PI_Serial::checkQFLAG',
  ];
  const constants = source.match(/constexpr unsigned long kPiMinConnectionHoldMs[^;]+;\nconstexpr unsigned long kPiMaxConnectionHoldMs[^;]+;/);
  if (!constants) throw Error('Missing production connection limits');
  if (queries) methods.push('PI_Serial::PI_Serial', 'PI_Serial::~PI_Serial',
    'String PI_Serial::sendCommand', 'bool PI_Serial::isReadOnlyPiQuery', 'bool PI_Serial::sendCustomCommand');

  // Snapshot/timing groups substitute command transport. Only the query group
  // compiles and asserts the actual protected command queue and classifier.
  const adapter = queries ? '' : `
PI_Serial::PI_Serial(HardwareSerial &serial, int rx, int tx) { my_serialIntf=&serial; _rxPin=rx; _txPin=tx; }
PI_Serial::~PI_Serial() = default;
String PI_Serial::sendCommand(String command) { customCommandBuffer=command; return command; }
bool PI_Serial::sendCustomCommand() {
    if (customCommandBuffer.isEmpty()) return false;
    get.raw.commandAnswer=requestData(customCommandBuffer);
    customCommandBuffer="";
    lastCustomCommandWasQuery=false;
    return true;
}
`;
  const unusedEnergy = '\n#ifndef TEST_PI_ENERGY\nEnergyBacklog::EnergyBacklog() {}\n#endif\n';
  harness.write('pi_state_generated.h', constants[0] + '\n' + methods.map(signature => extract(source, signature)).join('\n') + adapter + unusedEnergy);
  const service = harness.read('src/solar/SolarInverterService.cpp');
  harness.write('service_state_generated.h', [
    'void SolarInverterService::handleClientUpdate', 'void SolarInverterService::refreshRawState',
    'bool SolarInverterService::hasFreshBatterySample', 'BatterySampleTiming SolarInverterService::batterySampleTiming',
    'bool SolarInverterService::hasCommandAnswer', 'String SolarInverterService::consumeCommandAnswer',
  ].map(signature => extract(service, signature)).join('\n'));
  mqtt(harness);
}

module.exports = { mqtt, pi };
