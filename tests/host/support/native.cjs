'use strict';
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');

function run(command, args, options = {}) {
  const result = spawnSync(command, args, { stdio: 'inherit', ...options });
  if (result.error) throw result.error;
  if (result.status !== 0) throw Error(`${path.basename(command)} failed: ${result.status ?? result.signal}`);
  return result;
}

function extraCompilerFlags() {
  const flags = JSON.parse(process.env.HOST_CXXFLAGS_JSON || '[]');
  if (!Array.isArray(flags) || flags.some(flag => typeof flag !== 'string'))
    throw Error('HOST_CXXFLAGS_JSON must be a JSON array of compiler arguments');
  if (flags.some((flag, index) => /^(?:-D|\/D)NDEBUG(?:=|$)/.test(flag) ||
      (['-D', '/D'].includes(flag) && /^NDEBUG(?:=|$)/.test(flags[index + 1] || ''))))
    throw Error('Host regressions require enabled assertions');
  return flags;
}

function harness(group, { root = path.resolve(__dirname, '../../..') } = {}) {
  const tempRoot = path.resolve(os.tmpdir());
  const prefix = 'solar2mqtt-' + group + '-';
  const build = fs.mkdtempSync(path.join(tempRoot, prefix));
  const read = file => fs.readFileSync(path.join(root, file), 'utf8').replace(/\r\n/g, '\n');
  const write = (file, text) => {
    const target = path.join(build, file);
    fs.mkdirSync(path.dirname(target), { recursive: true });
    fs.writeFileSync(target, text);
  };
  const cleanup = () => {
    if (path.dirname(path.resolve(build)) !== tempRoot || !path.basename(build).startsWith(prefix))
      throw Error('Unexpected native build directory');
    fs.rmSync(build, { recursive: true, force: true });
  };
  const compile = (test, sources = [], { mocked = true, esp32Abi = false } = {}) => {
    const cxx = process.env.CXX || 'c++';
    const gcc = !/clang/i.test(path.basename(cxx)) && process.platform !== 'darwin';
    const json = process.env.ARDUINOJSON_INCLUDE || path.join(root, '.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
    const args = ['-std=c++17', '-Wall', '-Wextra', '-Werror', ...extraCompilerFlags(), '-UNDEBUG'];
    if (process.platform !== 'win32') args.push('-pthread');
    if (mocked) {
      args.push('-Wno-unused-variable', '-Wno-unused-parameter', '-Wno-missing-field-initializers');
      if (gcc) args.push('-Wno-switch-bool');
    }
    if (esp32Abi) args.push('-DSOLAR_TEST_ESP32_ABI=1');
    args.push('-DARDUINOJSON_USE_DOUBLE=1', '-DARDUINOJSON_USE_LONG_LONG=1', '-DSTRVERSION="test-version"',
      '-I' + build, '-I' + path.join(root, 'src'), '-I' + json,
      path.join(root, test), ...sources.map(file => path.join(root, file)));
    const executable = path.join(build, process.platform === 'win32' ? 'regression.exe' : 'regression');
    run(cxx, [...args, '-o', executable]);
    return executable;
  };
  return { root, build, read, write, cleanup, compile, run };
}

module.exports = { run, extraCompilerFlags, harness };
