const fs = require('node:fs'), os = require('node:os'), path = require('node:path');
const { spawnSync } = require('node:child_process');
function runTests(args = []) {
  const root = path.resolve(__dirname, '../..'), tempRoot = path.resolve(os.tmpdir());
  const build = fs.mkdtempSync(path.join(tempRoot, 'solar2mqtt-qpigs-'));
  function run(command, args) {
    const result = spawnSync(command, args, { stdio: 'inherit' });
    if (result.error) throw result.error;
    if (result.status !== 0) throw Error(path.basename(command) + ' failed: ' + result.status);
  }
  try {
    const driver = fs.readFileSync(path.join(root, 'src/PI_Serial/PI_Serial.h'), 'utf8').replace(/\r\n/g, '\n');
    const start = driver.indexOf('static inline int pi_split_fields('), end = driver.indexOf('\nclass PI_Serial');
    if (start < 0 || end <= start) throw Error('Production field helpers not found');
    const types = fs.readFileSync(path.join(root, 'src/solar/SolarTypes.h'), 'utf8').replace(/\r\n/g, '\n');
    const protocolEnum = types.match(/enum protocol_type_t\s*\{[\s\S]*?\};/);
    if (!protocolEnum) throw Error('Production protocol definitions not found');
    const predicates = ['isPi30LikeProtocol', 'isClassicPiStatusProtocol'].map(name => {
      const found = types.match(new RegExp('inline bool ' + name + '\\(protocol_type_t protocol\\)\\s*\\{[\\s\\S]*?\\n\\}'));
      if (!found) throw Error('Production predicate not found: ' + name);
      return found[0];
    });
    // This test is one translation unit; dropping the table's inline linkage
    // also permits older native MinGW compilers without inline-variable support.
    const protocolNames = types.slice(types.indexOf('inline const char *const protocolStrings[]'), types.indexOf('inline bool protocolFromString('))
      .replace('inline const char *const protocolStrings[]', 'const char *const protocolStrings[]');
    if (!protocolNames.includes('protocolToString')) throw Error('Production protocol names not found');
    fs.writeFileSync(path.join(build, 'pi_qpigs_helpers_generated.h'), protocolEnum[0] + '\n' + protocolNames + '\n' + predicates.join('\n') + '\n' + driver.slice(start, end));
    const implementation = fs.readFileSync(path.join(root, 'src/PI_Serial/PI_Serial.cpp'), 'utf8').replace(/\r\n/g, '\n');
    const modeMapper = implementation.match(/char \*PI_Serial::getModeDesc\(char mode\)[\s\S]*?\n\}/);
    if (!modeMapper) throw Error('Production mode mapper not found');
    const detectionHelpers = ['int countDelimitedFields', 'bool hasFlagSymbol', 'bool isNumericProtocolId', 'bool hasRevoSignature',
      'bool PI_Serial::isValidResponse', 'void PI_Serial::refineProtocol'].map(signature => {
      const begin = implementation.indexOf(signature + '('), end = implementation.indexOf('\n}', begin);
      if (begin < 0 || end < begin) throw Error('Production detection helper not found: ' + signature);
      return implementation.slice(begin, end + 2);
    });
    // refineProtocol currently has an unused qpigs2Fields local. Preserve the
    // complete production function and suppress only that pre-existing warning.
    fs.writeFileSync(path.join(build, 'pi_qpigs_mode_generated.h'), modeMapper[0] + '\n' +
      '#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-variable"\n' +
      detectionHelpers.join('\n') + '\n#pragma GCC diagnostic pop\n');
    const json = process.env.ARDUINOJSON_INCLUDE || path.join(root, '.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
    const executable = path.join(build, process.platform === 'win32' ? 'test_qpigs.exe' : 'test_qpigs');
    const compilerFlags = JSON.parse(process.env.HOST_CXXFLAGS_JSON || '[]');
    if (!Array.isArray(compilerFlags) || compilerFlags.some(flag => typeof flag !== 'string')) {
      throw Error('HOST_CXXFLAGS_JSON must be a JSON array of compiler arguments');
    }
    run(process.env.CXX || 'c++', ['-std=c++17', ...compilerFlags, '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
      '-DARDUINOJSON_USE_DOUBLE=1', '-DARDUINOJSON_USE_LONG_LONG=1',
      '-I' + path.join(root, 'src'), '-I' + json, '-I' + build,
      path.join(__dirname, 'test_qpigs.cpp'), '-o', executable]);
    run(executable, args);
  } finally {
    if (path.dirname(path.resolve(build)) !== tempRoot || !path.basename(build).startsWith('solar2mqtt-qpigs-')) throw Error('Unexpected temporary build path');
    fs.rmSync(build, { recursive: true, force: true });
  }

}
module.exports = {runTests};
if (require.main === module) {
  try { runTests(process.argv.slice(2)); }
  catch (error) { console.error(error); process.exitCode = 1; }
}
