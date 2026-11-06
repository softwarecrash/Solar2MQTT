'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const html=fs.readFileSync(path.resolve(__dirname,'../../src/webUI/firmware.html'),'utf8');
const release=JSON.parse(fs.readFileSync(path.join(__dirname,'fixtures/github-release-V2.0.4.json')));
const tick=()=>new Promise(resolve=>setImmediate(resolve));
async function fixture(){
 const elements=new Map(),calls=[];let response={state:'idle',busy:false};
 const element=id=>{if(!elements.has(id))elements.set(id,{style:{},textContent:'',disabled:false,handlers:{},addEventListener(name,fn){this.handlers[name]=fn;}});return elements.get(id);};
 const context=vm.createContext({document:{getElementById:element,addEventListener(){}},console,
  setInterval(){return 1;},clearInterval(){},setTimeout(){return 1;},clearTimeout(){},
  fetch:async(url,options={})=>{calls.push({url,method:options.method||'GET'});if(response instanceof Error)throw response;return {ok:!response.httpError,status:response.httpError||200,json:async()=>{if(response.jsonError)throw Error('Invalid JSON');return response;}};},
  location:{href:''},URL,Blob});
 for(const match of html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g))vm.runInContext(match[1],context,{filename:'src/webUI/firmware.html'});
 await tick();calls.length=0;
 return {context,calls,element,setResponse:r=>{response=r;},click:async()=>{const b=element('otaUpdateBtn');if(!b.disabled)await b.handlers.click();}};
}
async function test(){
 const f=await fixture(),{context:c,element:e,calls}=f;
 c.updateOtaUi({state:'error',busy:false,error:'Release response empty'});
 assert.equal(e('fwStatus').textContent,'Release response empty');assert.equal(e('otaUpdateBtn').disabled,false);
 assert.equal(e('otaUpdateBtn').textContent,'Retry update check');
 f.setResponse({state:'checking',busy:true});await f.click();
 assert.deepEqual(calls,[{url:'/ota/check',method:'POST'}]);assert.equal(e('otaUpdateBtn').disabled,true);
 for(const reply of [{httpError:503},new Error('offline'),{jsonError:true}]){
  calls.length=0;f.setResponse(reply);await c.requestOtaCheck();
  assert(e('fwStatus').textContent);assert.equal(e('otaUpdateBtn').disabled,false);
  assert(!calls.some(r=>r.url==='/ota/update'));
 }
 calls.length=0;c.updateOtaUi({state:'up_to_date',current:'0.0.0-dev-2.0.6-RC1',latest:release.tag_name.slice(1)});
 assert.equal(e('otaUpdateBtn').disabled,true);await f.click();assert.equal(calls.length,0);
 c.updateOtaUi({state:'update_available',latest:release.tag_name.slice(1),busy:false});
 assert.equal(e('otaUpdateBtn').textContent,'Update to v2.0.4');
 f.setResponse({state:'downloading',busy:true,progress:42,bytesDone:42,bytesTotal:100});await f.click();
 assert.deepEqual(calls,[{url:'/ota/update',method:'POST'}]);assert.equal(e('otaUpdateBtn').disabled,true);
 assert.equal(e('otaProgressWrap').style.display,'block');assert.equal(e('otaProgressBar').style.width,'42.0%');
 const g=await fixture();g.setResponse({httpError:401});await g.context.fetchOtaStatus();
 assert.equal(g.element('fwStatus').textContent,'Update status failed: 401');assert.equal(g.element('otaUpdateBtn').disabled,false);
 g.context.updateOtaUi({state:'error',error:'<script>untrusted response</script>'});
 assert.equal(g.element('fwStatus').textContent,'<script>untrusted response</script>');
 console.log('PASS: actual firmware page script shows errors, retries only checks, blocks unavailable updates and preserves update/progress behavior.');
}
module.exports={test};
if(require.main===module)test().catch(e=>{console.error(e);process.exitCode=1;});
