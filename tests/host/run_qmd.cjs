'use strict';

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');

const root = path.resolve(__dirname, '../..');
const tempRoot = path.resolve(os.tmpdir());
const build = fs.mkdtempSync(path.join(tempRoot, 'solar2mqtt-qmd-'));

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
    const helpers = [
        definition(types, 'enum protocol_type_t', '\n};'),
        definition(types, 'inline bool isPi30LikeProtocol'),
        definition(source('src/PI_Serial/PI_Serial.h'), 'static inline int pi_split_fields'),
        definition(source('src/PI_Serial/PI_Serial.cpp'), 'String sanitizePiReplyText')
    ];
    fs.writeFileSync(path.join(build, 'qmd_helpers.h'), helpers.join('\n'));
    const flags = JSON.parse(process.env.HOST_CXXFLAGS_JSON || '[]');
    if (!Array.isArray(flags) || flags.some(flag => typeof flag !== 'string'))
        throw Error('HOST_CXXFLAGS_JSON must be an array of compiler arguments');
    const json = process.env.ARDUINOJSON_INCLUDE || path.join(root, '.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
    const binary = path.join(build, process.platform === 'win32' ? 'test_qmd.exe' : 'test_qmd');
    run(process.env.CXX || 'c++', ['-std=c++17', ...flags, '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
        '-I' + path.join(root, 'src'), '-I' + json, '-I' + build,
        path.join(__dirname, 'test_qmd.cpp'), '-o', binary]);
    run(binary, []);
} finally {
    if (path.dirname(path.resolve(build)) !== tempRoot || !path.basename(build).startsWith('solar2mqtt-qmd-'))
        throw Error('Unexpected temporary build directory');
    fs.rmSync(build, {recursive: true, force: true});
}
