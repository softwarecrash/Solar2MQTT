'use strict';
const path=require('path');
const {harness}=require('./support/native.cjs');
function runTests(options={}){
 const h=harness('snapshot-memory',options);
 try{
  require('./snapshot_memory_support.cjs')(h);
  const executable=h.compile('tests/host/test_snapshot_memory.cpp', ['src/core/Ds18b20Service.cpp', 'src/core/InternalTemperatureService.cpp']);
  h.run(executable,['status-gk','status-max'].map(n=>path.join(h.root,'tests/host/fixtures',n+'.json')));
 }finally{h.cleanup();}
}
module.exports={runTests};
if(require.main===module)runTests();
