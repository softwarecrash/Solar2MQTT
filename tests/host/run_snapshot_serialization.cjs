'use strict';
const fs=require('node:fs'),path=require('node:path');
const {harness}=require('./support/native.cjs');
function runTests(options={}){
 const h=harness('snapshot-serialization',options);
 try{
  require('./snapshot_memory_support.cjs')(h);
  if(process.env.H1_READER_BASELINE==='1'){
   const {extract}=require('./support/extract.cjs');
   const old=fs.readFileSync(path.resolve(h.root,'../baseline/src/core/WebServerHandler.cpp'),'utf8').replace(/\r\n/g,'\n');
   const legacy=extract(old,'bool serializeCompleteJson').replace('bool serializeCompleteJson','bool baselineSerializeCompleteJson');
   const production=path.join(h.build,'production-snapshot.inc'),source=fs.readFileSync(production,'utf8');
   const current=extract(source,'bool SolarState::serializeSnapshot');
   const baseline='bool SolarState::serializeSnapshot(String &output){output="";JsonDocument copy;if(!snapshotTo(copy))return false;return baselineSerializeCompleteJson(copy,output);}';
   fs.writeFileSync(production,'#include "core/JsonOutput.h"\n'+legacy+'\n'+source.replace(current,baseline));
  }
  const file=path.join(h.build,'Arduino.h');let s=fs.readFileSync(file,'utf8');
  s=s.replace('class String {','struct SnapshotStringFaults {bool reserveFailure=false;size_t writeBudget=SIZE_MAX,reserves=0;std::function<void()> onReserve;};\nextern SnapshotStringFaults outputFault;\nclass String {');
  s=s.replace('bool reserve(size_t n){text.reserve(n);return true;}','bool reserve(size_t n){++outputFault.reserves;if(outputFault.onReserve)outputFault.onReserve();if(outputFault.reserveFailure)return false;text.reserve(n);return true;}');
  s=s.replace('size_t write(uint8_t c){text.push_back(static_cast<char>(c));return 1;}','size_t write(uint8_t c){return write(&c,1);}');
  s=s.replace('size_t write(const uint8_t *p,size_t n){text.append(reinterpret_cast<const char *>(p),n);return n;}','size_t write(const uint8_t *p,size_t n){const size_t accepted=std::min(n,outputFault.writeBudget);if(outputFault.writeBudget!=SIZE_MAX)outputFault.writeBudget-=accepted;text.append(reinterpret_cast<const char *>(p),accepted);return accepted;}');
  if(!s.includes('++outputFault.reserves')||!s.includes('outputFault.writeBudget'))throw Error('Missing snapshot String faults');
  fs.writeFileSync(file,s);
  const executable=h.compile('tests/host/test_snapshot_serialization.cpp');
  h.run(executable,['status-gk','status-max'].map(n=>path.join(h.root,'tests/host/fixtures',n+'.json')),{timeout:15000});
 }finally{h.cleanup();}
}
module.exports={runTests};if(require.main===module)runTests();
