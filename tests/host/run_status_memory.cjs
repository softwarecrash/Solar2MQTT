'use strict';
const path=require('node:path');
const {harness}=require('./support/native.cjs');
function runTests(options={}){
 const h=harness('status-memory',options);
 try{
  require('./status_memory_support.cjs')(h);
  const executable=h.compile('tests/host/test_status_memory.cpp');
  h.run(executable,['status-max','status-gk'].map(name=>path.join(h.root,'tests/host/fixtures',name+'.json')).concat('--require-memory-safe'));
 }finally{h.cleanup();}
}
module.exports={runTests};
if(require.main===module)runTests();
