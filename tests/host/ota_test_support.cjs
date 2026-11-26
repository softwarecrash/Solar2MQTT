'use strict';
module.exports = h => {
  require('./ota_support/mocks.cjs')(h);
  const fs=require('node:fs'),path=require('node:path');
  // FreeRTOS zero-timeout takes are nonblocking; ordinary takes still wait.
  const semaphore=path.join(h.build,'freertos/semphr.h');
  let sem=fs.readFileSync(semaphore,'utf8');
  sem=sem.replace('inline void xSemaphoreTake(SemaphoreHandle_t p,unsigned long){p->lock();}',
    'constexpr int pdTRUE=1;\ninline int xSemaphoreTake(SemaphoreHandle_t p,unsigned long timeout){if(timeout==0)return p->try_lock()?pdTRUE:0;p->lock();return pdTRUE;}');
  if(!sem.includes('p->try_lock()'))throw Error('Semaphore adapter did not match');
  fs.writeFileSync(semaphore,sem);
  const arduino=path.join(h.build,'Arduino.h');
  fs.appendFileSync(arduino, String.raw`
inline bool isAlpha(unsigned char c){return std::isalpha(c)!=0;}
ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE
template<>class Reader<const String,void>{
 const char *p;public:explicit Reader(const String &s):p(s.c_str()){}int read(){return *p?static_cast<unsigned char>(*p++):-1;}
 size_t readBytes(char *out,size_t n){size_t i=0;for(;i<n&&*p;++i)out[i]=*p++;return i;}
};
ARDUINOJSON_END_PRIVATE_NAMESPACE
// Methods provided by the real Arduino String; no OTA behavior is substituted.
`);
  let text=fs.readFileSync(arduino,'utf8');
  text=text.replace(' void trim(){',' void toUpperCase(){std::transform(text.begin(),text.end(),text.begin(),[](unsigned char c){return std::toupper(c);});}\n int compareTo(const String &s)const{return text.compare(s.text);}\n int indexOf(const String &s)const{auto n=text.find(s.text);return n==std::string::npos?-1:int(n);}\n void trim(){');
  fs.writeFileSync(arduino,text);
  h.write('OtaMock.h',String.raw`#pragma once
#include <Arduino.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <vector>
struct OtaMock {
 std::string response,url,userAgent;int code=200,reportedSize=-2;size_t offset=0,failAt=SIZE_MAX;
 bool beginOk=true,largeStringUnavailable=false,http10=false,closed=false,taskFail=false,eager=false;
 size_t gapAt=SIZE_MAX;uint32_t gapDuration=0,gapStartedAt=0;bool gapStarted=false,gapFinished=false;
 std::vector<int> httpCodes;unsigned clientStops=0;
 unsigned getStringCalls=0,streamCalls=0,getCalls=0,taskCount=0,deletes=0,updateBegins=0;
 uint32_t timeout=0,connectTimeout=0,handshakeSeconds=0;
};
extern OtaMock ota;
using TaskHandle_t=void *;constexpr int pdPASS=1;constexpr int HTTPC_FORCE_FOLLOW_REDIRECTS=1;
constexpr uint32_t UPDATE_SIZE_UNKNOWN=0xffffffffUL;
struct TestTask {void(*fn)(void *);void *param;std::thread worker;bool finished=false;std::mutex mutex;std::condition_variable cv;};
extern std::vector<std::unique_ptr<TestTask>> tasks;
inline int xTaskCreate(void(*fn)(void *),const char *,uint32_t,void *param,unsigned,TaskHandle_t *handle){
 ++ota.taskCount;if(ota.taskFail)return 0;
 tasks.push_back(std::unique_ptr<TestTask>(new TestTask{fn,param}));auto *t=tasks.back().get();*handle=t;
 if(ota.eager){t->worker=std::thread([t]{t->fn(t->param);{std::lock_guard<std::mutex> l(t->mutex);t->finished=true;}t->cv.notify_one();});
 std::unique_lock<std::mutex> l(t->mutex);t->cv.wait_for(l,std::chrono::milliseconds(20),[t]{return t->finished;});}
 return pdPASS;
}
inline void vTaskDelete(void *){++ota.deletes;}
inline void vTaskDelay(unsigned n){delay(n);}
struct ReplyStream {
 int available(){return ota.offset<ota.failAt?int(ota.response.size()-ota.offset):0;}
 int read(){if(ota.offset==ota.gapAt&&!ota.gapFinished){if(!ota.gapStarted){ota.gapStarted=true;ota.gapStartedAt=millis();}if(static_cast<uint32_t>(millis()-ota.gapStartedAt)<ota.gapDuration)return -1;ota.gapFinished=true;}if(ota.offset>=ota.response.size()||ota.offset>=ota.failAt)return -1;return static_cast<unsigned char>(ota.response[ota.offset++]);}
 size_t readBytes(char *p,size_t n){size_t i=0;for(;i<n;++i){int c=read();if(c<0)break;p[i]=char(c);}return i;}
 size_t readBytes(uint8_t *p,size_t n){return readBytes(reinterpret_cast<char *>(p),n);}
};
using NetworkClient=ReplyStream;
class WiFiClientSecure {public:void stop(){++ota.clientStops;}void setInsecure(){}void setTimeout(uint32_t ms){ota.timeout=ms;}void setHandshakeTimeout(uint32_t s){ota.handshakeSeconds=s;}};
class HTTPClient {public:
 void useHTTP10(bool v){ota.http10=v;}void setFollowRedirects(int){}void setTimeout(uint32_t n){ota.timeout=n;}
 void setConnectTimeout(uint32_t n){ota.connectTimeout=n;}void setUserAgent(const char *s){ota.userAgent=s;}
 void addHeader(const char *,const char *){}
 bool begin(WiFiClientSecure &,const String &url){ota.url=url.text;return ota.beginOk;}
 int GET(){const auto i=ota.getCalls++;return i<ota.httpCodes.size()?ota.httpCodes[i]:ota.code;}
 int getSize(){return ota.reportedSize==-2?int(ota.response.size()):ota.reportedSize;}
 String getString(){++ota.getStringCalls;return ota.largeStringUnavailable?String():String(ota.response);}
 ReplyStream &getStream(){++ota.streamCalls;static ReplyStream stream;return stream;}
 ReplyStream *getStreamPtr(){return &getStream();}
 bool connected(){return ota.offset<ota.response.size()&&ota.offset<ota.failAt;}
 void end(){ota.closed=true;}
};
struct TestUpdate {bool isRunning(){return false;}bool begin(uint32_t){++ota.updateBegins;return true;}size_t write(const uint8_t *,size_t n){return n;}bool end(bool){return true;}void abort(){}};
extern TestUpdate Update;
`);
  h.write('WiFiClientSecure.h','#pragma once\n#include "OtaMock.h"\n');
  h.write('HTTPClient.h','#pragma once\n#include "OtaMock.h"\n');
  h.write('Update.h','#pragma once\n#include "OtaMock.h"\n');
  h.write('freertos/task.h','#pragma once\n#include "OtaMock.h"\n');
  h.write('freertos/FreeRTOS.h','#pragma once\n#include "OtaMock.h"\n');
  // Arduino.h normally exposes the FreeRTOS task API.
  fs.appendFileSync(arduino,'\n#include "OtaMock.h"\n');
};
