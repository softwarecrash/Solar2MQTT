'use strict';
const fs=require('fs'),path=require('path');
const {harness}=require('../support/native.cjs');
function runTests(){
const h=harness('firmware-writer');
try{
 require('../ota_test_support.cjs')(h);
 const mock=path.join(h.build,'OtaMock.h'); let s=fs.readFileSync(mock,'utf8');
 const a=s.indexOf('struct TestUpdate {'), b=s.indexOf('extern TestUpdate Update;',a);
 if(a<0||b<0)throw Error('missing Update mock');
 s=s.slice(0,a)+h.read('tests/host/writer/update-substitute.hpp')+'\n'+s.slice(b); fs.writeFileSync(mock,s);
 const web=h.read('src/core/WebServerHandler.cpp');
 const start=web.indexOf('    struct ManualUpdateContext'),end=web.indexOf('    _server.on("/update_firmware"',start);
 if(start<0||end<0)throw Error('missing production callbacks');
 h.write('production-callbacks.inc',web.slice(start,end));
 const old=process.env.HOST_CXXFLAGS_JSON;
 process.env.HOST_CXXFLAGS_JSON=JSON.stringify([...JSON.parse(old||'[]'),'-DFIXED=1']);
 try{const binary=h.compile('tests/host/writer/test.cpp',['src/core/GitHubOtaUpdater.cpp']);h.run(binary);}
 finally{if(old===undefined)delete process.env.HOST_CXXFLAGS_JSON;else process.env.HOST_CXXFLAGS_JSON=old;}

}finally{h.cleanup();}

}
module.exports={runTests};
if(require.main===module)runTests();
