function extract(text, signature) {
    const start = text.indexOf(signature + '(');
    const end = text.indexOf('\n}', start);
    if (start < 0 || end < 0) throw Error('Missing production definition ' + signature);
    return text.slice(start, end + 2);
}

module.exports=({write,read})=>{
 write('DownloadMock.h',String.raw`#pragma once
#include <Arduino.h>
#include <vector>
struct DownloadMock {bool beginOk=true,updateBeginOk=true,writeOk=true,endOk=true,stalled=false,nullStream=false;int code=200,reportedSize=-2;size_t offset=0,disconnectAt=SIZE_MAX;unsigned aborted=0;std::vector<uint8_t> bytes=std::vector<uint8_t>(2051,0x5a);};
extern DownloadMock download;
constexpr int HTTPC_FORCE_FOLLOW_REDIRECTS=1;constexpr uint32_t UPDATE_SIZE_UNKNOWN=0xffffffffUL;
inline void vTaskDelay(unsigned n){delay(n);}
struct DownloadStream {
 size_t available(){return download.stalled||download.offset>=download.disconnectAt?0:download.bytes.size()-download.offset;}
 size_t readBytes(uint8_t *p,size_t n){n=std::min(n,available());std::memcpy(p,download.bytes.data()+download.offset,n);download.offset+=n;return n;}
};
class HTTPClient {public:
 void setFollowRedirects(int){}void setTimeout(uint32_t){}void setUserAgent(const char *){}
 void useHTTP10(bool){}void setConnectTimeout(uint32_t){}
 bool begin(WiFiClientSecure &,const String &){return download.beginOk;}
 int GET(){return download.code;}int getSize(){return download.reportedSize==-2?int(download.bytes.size()):download.reportedSize;}
 bool connected(){return download.offset<download.bytes.size()&&download.offset<download.disconnectAt;}void end(){}
 DownloadStream *getStreamPtr(){static DownloadStream stream;return download.nullStream?nullptr:&stream;}
};
struct FakeUpdate {
 bool running=false;
 bool isRunning(){return running;}
 bool begin(uint32_t){return running=download.updateBeginOk;}
 size_t write(const uint8_t *,size_t n){return download.writeOk?n:0;}
 bool end(bool){if(download.endOk)running=false;return download.endOk;}void abort(){running=false;++download.aborted;}
};
extern FakeUpdate Update;
`);
 write('freertos/FreeRTOS.h','#pragma once');
 write('Update.h','#pragma once\n#include "DownloadMock.h"\n');
 const ota=read('src/core/GitHubOtaUpdater.cpp');
 const constants=ota.match(/constexpr uint32_t kHttpTimeoutMs[^;]+;\nconstexpr const char \*kUserAgent[^;]+;/);
 if(!constants)throw Error('Missing OTA constants');
 const methods=['GitHubOtaUpdater::GitHubOtaUpdater','void GitHubOtaUpdater::begin','void GitHubOtaUpdater::doDownload','void GitHubOtaUpdater::scheduleRestart',
  'bool GitHubOtaUpdater::consumeRestartRequest','void GitHubOtaUpdater::setState','void GitHubOtaUpdater::lock','void GitHubOtaUpdater::unlock'];
 write('restart_generated.h',constants[0]+'\n'+methods.map(s=>extract(ota,s)).join('\n'));
 write('restart_main_generated.h',extract(read('src/main.cpp'),'void processPendingRestart'));
 write('restart_flush_generated.h',extract(read('src/core/MqttHandler.cpp'),'void MqttHandler::flushPersistentEnergy'));
 // The shared native client normally has no TLS calls. Only its interface to
 // this production download function is needed; no network access occurs.
 write('WiFiClientSecure.h','#pragma once\n#include <WiFiClient.h>\nclass WiFiClientSecure:public Client {public:void setInsecure(){}void setHandshakeTimeout(uint32_t){}};\n');
 return {test:'test_restart.cpp',sources:[]};
};
