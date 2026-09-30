const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('src/webUI/app.js', 'utf8');
const code = source.slice(source.indexOf('async function waitForPowmrHunt('), source.indexOf('async function runConsoleCommand('));
async function test(cancel, failFirst) {
  const commands = []; let removed = false; let click; let polls = 0;
  const answer = text => ({ RawData: { CommandAnswer: text } });
  const context = {
    URLSearchParams,
    getCommandAnswerValue: d => d.RawData.CommandAnswer,
    setText() {}, showNotice() {},
    byId: () => ({ appendChild() { if (cancel) click(); } }),
    document: { createElement: () => ({ addEventListener(_, fn) { click=fn; }, remove() { removed=true; } }) },
    window: { setTimeout(fn) { fn(); } },
    fetchJson: async (_, args) => {
      const command=args.body.get('command'); commands.push(command);
      if (failFirst && commands.length===1) throw Error('temporary failure');
    },
    waitForCommandAnswer: async () => answer(++polls===1 && !cancel ? 'POWMR_HUNT RUNNING' : 'POWMR_HUNT DONE'),
  };
  vm.createContext(context); vm.runInContext(code, context);
  await context.waitForPowmrHunt(answer('POWMR_HUNT RUNNING'));
  assert(removed);
  assert(commands.every(c => ['powmr hunt status','powmr hunt cancel','powmr hunt result'].includes(c)));
  assert.equal(commands.at(-1),'powmr hunt result');
  assert.equal(commands[0],cancel ? 'powmr hunt cancel' : 'powmr hunt status');
}
(async () => { await test(false,false); await test(false,true); await test(true,false); })().catch(e=>{console.error(e);process.exit(1);});
