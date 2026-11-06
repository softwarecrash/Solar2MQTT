const fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {spawnSync}=require('node:child_process');
const {extraCompilerFlags}=require('./support/native.cjs');

function runTests(args = []) {
const root=path.resolve(__dirname,'../..'),tempRoot=path.resolve(os.tmpdir());
const build=fs.mkdtempSync(path.join(tempRoot,'solar2mqtt-qpigs2-'));
function run(command,args){const result=spawnSync(command,args,{stdio:'inherit'});if(result.error)throw result.error;if(result.status!==0)throw Error(path.basename(command)+' failed: '+result.status);}
try{
  const driver=fs.readFileSync(path.join(root,'src/PI_Serial/PI_Serial.h'),'utf8').replace(/\r\n/g,'\n');
  const start=driver.indexOf('static inline int pi_split_fields('),end=driver.indexOf('\nclass PI_Serial');
  if(start<0||end<=start)throw Error('Production field helpers not found');
  const types=fs.readFileSync(path.join(root,'src/solar/SolarTypes.h'),'utf8');
  const protocolEnum=types.match(/enum protocol_type_t\s*\{[\s\S]*?\};/);
  const predicate=types.match(/inline bool isPi30LikeProtocol\(protocol_type_t protocol\)\s*\{[\s\S]*?\n\}/);
  if(!protocolEnum||!predicate)throw Error('Production protocol definitions not found');
  fs.writeFileSync(path.join(build,'pi_qpigs2_helpers_generated.h'),protocolEnum[0]+'\n'+predicate[0]+'\n'+driver.slice(start,end));
  const json=process.env.ARDUINOJSON_INCLUDE||path.join(root,'.pio/libdeps/wemos_d1_mini32/ArduinoJson/src');
  const executable=path.join(build,process.platform==='win32'?'test_qpigs2.exe':'test_qpigs2');
  run(process.env.CXX||'c++',['-std=c++17',...extraCompilerFlags(),'-UNDEBUG','-Wall','-Wextra','-Werror','-DARDUINOJSON_USE_DOUBLE=1','-DARDUINOJSON_USE_LONG_LONG=1','-I'+path.join(root,'src'),'-I'+json,'-I'+build,path.join(__dirname,'test_qpigs2.cpp'),'-o',executable]);
  run(executable,args);
}finally{
  if(path.dirname(path.resolve(build))!==tempRoot||!path.basename(build).startsWith('solar2mqtt-qpigs2-'))throw Error('Unexpected temporary build path');
  fs.rmSync(build,{recursive:true,force:true});
}

}

module.exports = {runTests};
if (require.main === module) {
  try { runTests(process.argv.slice(2)); }
  catch(error) { console.error(error); process.exitCode=1; }
}
