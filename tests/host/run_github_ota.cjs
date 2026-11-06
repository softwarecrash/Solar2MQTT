'use strict';
const path=require('node:path');
const {harness}=require('./support/native.cjs');
function runTests(args=[],options={}){
 const h=harness('github-ota',options);
 const prior=process.env.HOST_CXXFLAGS_JSON;
 try{
  if(process.platform!=='darwin'&&!/clang/i.test(path.basename(process.env.CXX||'c++')))
   process.env.HOST_CXXFLAGS_JSON=JSON.stringify([...JSON.parse(prior||'[]'),'-DTEST_WRAP_ALLOC=1','-Wl,--wrap=malloc','-Wl,--wrap=realloc']);
  require('./ota_test_support.cjs')(h);
  const binary=h.compile('tests/host/test_github_ota.cpp',['src/core/GitHubOtaUpdater.cpp']);
  h.run(binary,[path.join(h.root,'tests/host/fixtures/github-release-V2.0.4.json'),...args]);
 }finally{if(prior===undefined)delete process.env.HOST_CXXFLAGS_JSON;else process.env.HOST_CXXFLAGS_JSON=prior;h.cleanup();}
}
module.exports={runTests};
if(require.main===module)runTests(process.argv.slice(2));
