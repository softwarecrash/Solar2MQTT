'use strict';
const fs=require('node:fs'),path=require('node:path');
const {harness}=require('./support/native.cjs');
function runTests(options={}){
 const h=harness('discovery-memory',options);
 try{
  require('./support/mocks.cjs')(h);
  require('./support/production.cjs').mqtt(h,{api:true,fullDiscovery:true});
  if(process.env.DISCOVERY_MEMORY_BASELINE==='1'){
   const {extract}=require('./support/extract.cjs');
   const baseline=fs.readFileSync(path.resolve(h.root,'../baseline/src/core/MqttHandler.cpp'),'utf8').replace(/\r\n/g,'\n');
   const file=path.join(h.build,'mqtt_state_generated.h');let generated=fs.readFileSync(file,'utf8');
   for(const name of ['publishHaDiscovery','publishHaSection','publishHaEspInternalTemperature','publishHaDs18b20','publishHaPowMrSettings']){
    const signature='void MqttHandler::'+name;
    generated=generated.replace(extract(generated,signature),extract(baseline,signature));
   }
   fs.writeFileSync(file,generated);
  }
  // Read-only instrumentation of the real collector at its otherwise unused
  // non-forced return. This detects accidental growth below the old reserve size.
  const generatedFile=path.join(h.build,'mqtt_state_generated.h');let generated=fs.readFileSync(generatedFile,'utf8');
  const {extract}=require('./support/extract.cjs');
  const method=extract(generated,'void MqttHandler::publishHaDiscovery');
  const anchor='    if (!force)\n    {\n        return;\n    }';
  if(method.split(anchor).length!==2)throw Error('Missing incremental collector return');
  generated=generated.replace(method,method.replace(anchor,'    if (!force)\n    {\n        observeDiscoveryCollector(force, currentTopics);\n        return;\n    }'));
  if(process.env.DISCOVERY_COLLECTOR_MUTATION==='1'){
   const guard='        if (force)\n            appendTopicIfMissing(currentTopics, topic);';
   if(!generated.includes(guard))throw Error('Missing collector mutation guard');
   generated=generated.replace(guard,'        appendTopicIfMissing(currentTopics, topic);');
  }
  fs.writeFileSync(generatedFile,'void observeDiscoveryCollector(bool, const std::vector<String> &);\n'+generated);
  const executable=h.compile('tests/host/test_discovery_memory.cpp',['src/core/SolarState.cpp','src/core/EnergyBacklog.cpp']);
  h.run(executable,['status-gk','status-max'].map(n=>path.join(h.root,'tests/host/fixtures',n+'.json')),{timeout:15000});
 }finally{h.cleanup();}
}
module.exports={runTests};if(require.main===module)runTests();
