'use strict';
const assert = require('node:assert/strict');
const { extract } = require('./support/extract.cjs');
const { extraCompilerFlags } = require('./support/native.cjs');

async function test() {
  const cases = [
    ['int plain() { return 1; }', 'int plain'],
    ['int nested() { if (true) { return 1; } return 0; }', 'int nested'],
    ['int quoted() { const char *s="}\\\"{"; return 1; }', 'int quoted'],
    ["int character() { char c='}'; return c; }", 'int character'],
    ['int comments() { /* } */ // }\n return 1; }', 'int comments'],
    ['int raw() { const char *s=R"tag({\"})tag"; return 1; }', 'int raw'],
    ['Thing::Thing() : first{1}, second(2) { if (true) { run(); } }', 'Thing::Thing'],
    ['int spaces()\n    {\n      return [](){ return 1; }();\n    }', 'int spaces'],
    ["int separators() { return 1'000; }", 'int separators'],
  ];
  for (const [definition, signature] of cases) {
    const source = '// int fake() { }\n' + definition + '\nint after() { return 2; }';
    assert.equal(extract(source, signature), definition);
  }
  assert.throws(() => extract('int first() {}', 'int absent'), /Missing/);
  assert.throws(() => extract('int first() {', 'int first'), /Unbalanced/);
  assert.throws(() => extract('int first();\nint second() {}', 'int first'), /body/);
  assert.throws(() => extract('int first() {}\nint first(int x) {}', 'int first'), /Ambiguous/);
  const previous = process.env.HOST_CXXFLAGS_JSON;
  try {
    process.env.HOST_CXXFLAGS_JSON = '["-m32"]';
    assert.deepEqual(extraCompilerFlags(), ['-m32']);
    for (const value of ['{}', '[1]', '["-DNDEBUG"]', '["-D","NDEBUG"]']) {
      process.env.HOST_CXXFLAGS_JSON = value;
      assert.throws(extraCompilerFlags);
    }
  } finally {
    if (previous === undefined) delete process.env.HOST_CXXFLAGS_JSON;
    else process.env.HOST_CXXFLAGS_JSON = previous;
  }
  assert.throws(() => require('./run_state_cycles.cjs').runGroup('missing'), /Unknown/);
  for (const args of [['all', 'energy'], ['portable', 'energy'], ['--list', 'pi'], ['missing']])
    await assert.rejects(require('./run.cjs').main(args), /alone|Unknown/);
  console.log('PASS: brace-aware production extraction, constructors/literals/comments, required functions and assertion-safe compiler flags.');
}

module.exports = { test };
if (require.main === module) test().catch(error => { console.error(error); process.exitCode = 1; });
