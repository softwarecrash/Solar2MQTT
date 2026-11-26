'use strict';

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');

const root = path.resolve(__dirname, '../..');
const tempRoot = path.resolve(os.tmpdir());
const build = fs.mkdtempSync(path.join(tempRoot, 'solar2mqtt-pi-queries-'));

function source(file) {
    return fs.readFileSync(path.join(root, file), 'utf8').replace(/\r\n/g, '\n');
}

function definition(text, signature, ending = '\n}') {
    const start = text.indexOf(signature);
    const end = text.indexOf(ending, start);
    if (start < 0 || end < 0)
        throw Error('Missing production definition ' + signature);
    return text.slice(start, end + ending.length);
}

function run(command, args) {
    const result = spawnSync(command, args, {stdio: 'inherit'});
    if (result.error) throw result.error;
    if (result.status !== 0) throw Error(path.basename(command) + ' failed');
}

try {
    const types = source('src/solar/SolarTypes.h');
    const pi = source('src/PI_Serial/PI_Serial.cpp');
    const hold = pi.match(/constexpr unsigned long kPiConnectionHoldMs[^;]+;/);
    if (!hold) throw Error('Missing PI connection timeout');
    const predicates = ['isModbusProtocol', 'isPowMrProtocol', 'isPowMrPiHybridProtocol', 'isPi30LikeProtocol',
        'isClassicPiStatusProtocol', 'isRawOnlyPiProtocol'];
    fs.writeFileSync(path.join(build, 'pi_query_helpers.h'), [
        definition(types, 'enum protocol_type_t', '\n};'),
        definition(types, 'inline const char *const protocolStrings[]', '\n};')
            .replace('inline const char *const', 'const char *const'),
        definition(types, 'inline const char *protocolToString('),
        ...predicates.map(name => definition(types, 'inline bool ' + name + '(')),
        hold[0],
        definition(pi, 'void copyJsonObject(')
    ].join('\n'));
    const methods = ['bool PI_Serial::loop', 'bool PI_Serial::isModbus', 'bool PI_Serial::sendCustomCommand',
        'String PI_Serial::sendCommand', 'void PI_Serial::beginCycleBackup', 'void PI_Serial::restoreCycleBackup',
        'void PI_Serial::clearCycleBackup', 'void PI_Serial::markSuccessfulDynamicCycle'];
    if (pi.includes('bool PI_Serial::isReadOnlyPiQuery(')) methods.push('bool PI_Serial::isReadOnlyPiQuery');
    fs.writeFileSync(path.join(build, 'pi_query_methods.h'), methods.map(name => definition(pi, name + '(')).join('\n'));
    const flags = JSON.parse(process.env.HOST_CXXFLAGS_JSON || '[]');
    if (!Array.isArray(flags) || flags.some(flag => typeof flag !== 'string'))
        throw Error('HOST_CXXFLAGS_JSON must be an array of compiler arguments');
    const json = process.env.ARDUINOJSON_INCLUDE || path.join(root, '.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
    const binary = path.join(build, process.platform === 'win32' ? 'test_queries.exe' : 'test_queries');
    const compiler = process.env.CXX || 'c++';
    // The upstream loop switches on a boolean in two places.
    const loopWarnings = /clang/i.test(path.basename(compiler)) ? [] : ['-Wno-switch-bool'];
    run(compiler, ['-std=c++17', ...flags, ...loopWarnings, '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
        '-I' + json, '-I' + build, path.join(__dirname, 'test_pi_queries.cpp'), '-o', binary]);
    run(binary, []);
} finally {
    if (path.dirname(path.resolve(build)) !== tempRoot || !path.basename(build).startsWith('solar2mqtt-pi-queries-'))
        throw Error('Unexpected temporary build directory');
    fs.rmSync(build, {recursive: true, force: true});
}
