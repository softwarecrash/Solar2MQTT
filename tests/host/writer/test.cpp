#include <Arduino.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <type_traits>
#include <freertos/semphr.h>
#define private public
#include "core/GitHubOtaUpdater.h"
#undef private
#define private public
#include "core/FirmwareWriter.h"
#undef private
uint32_t testNow=100;
FakeEsp ESP;OtaMock ota;TestUpdate Update;
std::vector<std::unique_ptr<TestTask>> tasks;

int Serial=0;
bool g_pendingRestart=false;uint32_t g_restartAt=0;
static unsigned contextsFreed=0;
struct AsyncWebServerRequest {
 bool authorized=true; void *_tempObject=nullptr;
 ~AsyncWebServerRequest(){if(_tempObject)++contextsFreed;free(_tempObject);}
 int status=0;bool disconnectOnSend=false,disconnectedOnce=false;std::function<void()> disconnected;
 void requestAuthentication(){status=401;}
 void send(int code,const char *,const char *){status=code;if(disconnectOnSend)disconnect();}
 void onDisconnect(std::function<void()> fn){disconnected=std::move(fn);}
 void disconnect(){if(!disconnectedOnce){disconnectedOnce=true;if(disconnected)disconnected();}}
};
struct ManualHandler {
 std::function<void(AsyncWebServerRequest *)> finish;
 std::function<void(AsyncWebServerRequest *,const String &,size_t,uint8_t *,size_t,bool)> upload;
 bool isAuthorized(AsyncWebServerRequest *r){return r->authorized;}
 ManualHandler(){
#include "production-callbacks.inc"
  finish=finishUpdate;upload=uploadUpdate;
 }
 void chunk(AsyncWebServerRequest &r,const std::string &payload,size_t offset=0,bool final=true){
  std::vector<uint8_t> data(payload.begin(),payload.end());upload(&r,"firmware.bin",offset,data.data(),data.size(),final);
 }
};
static void reset(){
 assert(tasks.empty());ota=OtaMock{};ota.response="GGGG";Update=TestUpdate{};testNow=100;
 firmwareWriter.owner=nullptr;firmwareWriter.committed=false;g_pendingRestart=false;g_restartAt=0;
}
static void prepare(GitHubOtaUpdater &u){u.begin([]{return true;});u._assetUrl="https://offline.invalid/image";}
static void dispose(GitHubOtaUpdater &u){vSemaphoreDelete(static_cast<SemaphoreHandle_t>(u._lockHandle));u._lockHandle=nullptr;}
static void githubFirst(){
 reset();ManualHandler h;AsyncWebServerRequest manual;
 GitHubOtaUpdater u("o","r","2.0.3","board");prepare(u);
 Update.afterBegin=[&]{h.chunk(manual,"MM");h.finish(&manual);};u.doDownload();
#if FIXED
 assert(manual.status==409&&!g_pendingRestart);assert(Update.committed=="GGGG");
 assert(u._state==GitHubOtaUpdater::State::Success&&Update.writes==1&&Update.begins==1);
 AsyncWebServerRequest late;
 h.chunk(late,"late",0,true);h.finish(&late);
 assert(late.status==409&&Update.committed=="GGGG"&&Update.begins==1);
 assert(firmwareWriter.committed&&!g_pendingRestart);
 assert(u._restartAtMs-testNow==2500);
 assert(!u.consumeRestartRequest(u._restartAtMs-1)&&u.consumeRestartRequest(u._restartAtMs));
 std::cout<<"PASS github-first: rejected manual writer; GitHub bytes intact; 2500ms restart preserved\n";
#else
 assert(manual.status==200&&g_pendingRestart);assert(Update.committed=="MM");
 assert(u._state==GitHubOtaUpdater::State::Error&&Update.begins==2);
 std::cout<<"REPRO github-first: manual begin rejected, but manual bytes/finalize consumed GitHub session; manual reports success\n";
#endif
 dispose(u);
}
static void manualFirst(){
 reset();ManualHandler h;AsyncWebServerRequest manual;h.chunk(manual,"A",0,false);
 GitHubOtaUpdater u("o","r","2.0.3","board");prepare(u);u.doDownload();
 assert(u._state==GitHubOtaUpdater::State::Error&&Update.active&&Update.bytes=="A");
 h.chunk(manual,"B",1,true);h.finish(&manual);
 assert(manual.status==200&&Update.committed=="AB"&&g_pendingRestart&&g_restartAt==600);
#if FIXED
 assert(Update.begins==1);manual.disconnect();assert(Update.aborts==0&&firmwareWriter.committed);
#else
 assert(Update.begins==2);
#endif
 std::cout<<"PASS manual-first: manual bytes preserved; GitHub rejected; existing 500ms restart flags preserved\n";dispose(u);
}
static void twoManuals(){
 reset();ManualHandler h;AsyncWebServerRequest a,b;h.chunk(a,"A",0,false);h.chunk(b,"B",0,true);h.finish(&b);
#if FIXED
 assert(b.status==409&&Update.active&&Update.bytes=="A"&&!g_pendingRestart);
 h.chunk(a,"C",1,true);h.finish(&a);assert(a.status==200&&Update.committed=="AC");
 std::cout<<"PASS manual/manual: second request cannot write, finalize, or restart first request\n";
#else
 assert(b.status==200&&Update.committed=="AB"&&g_pendingRestart);
 std::cout<<"REPRO manual/manual: rejected second begin still writes and finalizes mixed AB bytes\n";
#endif
}
static void beginFailure(){
 reset();ManualHandler h;AsyncWebServerRequest r;Update.allocationFail=true;h.chunk(r,"",0,true);h.finish(&r);
 assert(!Update.active);
#if FIXED
 assert(r.status==500&&!g_pendingRestart&&Update.ends==0&&!firmwareWriter.owner);
 std::cout<<"PASS allocation failure: explicit per-request failure, no finalize/restart, ownership released\n";
#else
 assert(r.status==200&&g_pendingRestart&&Update.ends==1);
 std::cout<<"REPRO allocation failure: begin/end return false with error zero; finish responds success and schedules restart\n";
#endif
}
static void disconnect(){
 reset();ManualHandler h;AsyncWebServerRequest r;h.chunk(r,"A",0,false);r.disconnect();
#if FIXED
 assert(!Update.active&&Update.aborts==1&&!firmwareWriter.owner&&!g_pendingRestart);
 AsyncWebServerRequest next;h.chunk(next,"B");h.finish(&next);assert(next.status==200&&Update.committed=="B");
 std::cout<<"PASS disconnect: abort own incomplete session and allow next writer\n";
#else
 assert(Update.active&&Update.aborts==0);
 std::cout<<"REPRO disconnect: incomplete manual session remains running without cleanup\n";
#endif
}
static void unauthorized(){
 reset();ManualHandler h;AsyncWebServerRequest r;r.authorized=false;h.chunk(r,"X");h.finish(&r);
 assert(r.status==401&&Update.begins==0&&Update.writes==0&&!g_pendingRestart);
 std::cout<<"PASS unauthorized: preserved authentication; no firmware operations\n";
}
#if FIXED
static void completionAndFailure(){
 for(bool failWrite:{false,true}){
  reset();ManualHandler h;AsyncWebServerRequest r;Update.endFail=!failWrite;Update.writeFail=failWrite;h.chunk(r,"A");h.finish(&r);
  assert(r.status==500&&!Update.active&&!firmwareWriter.owner&&!g_pendingRestart);
 }
 reset();ManualHandler h;AsyncWebServerRequest winner,rejected;h.chunk(winner,"A");h.finish(&winner);winner.disconnect();
 const unsigned begins=Update.begins,aborts=Update.aborts;h.chunk(rejected,"B");h.finish(&rejected);rejected.disconnect();
 assert(rejected.status==409&&Update.begins==begins&&Update.aborts==aborts&&Update.committed=="A");
 reset();ManualHandler second;AsyncWebServerRequest abandoned,rejected2;second.chunk(abandoned,"A",0,false);second.chunk(rejected2,"B");rejected2.disconnect();
 assert(Update.active&&Update.bytes=="A"&&Update.aborts==0);abandoned.disconnect();assert(Update.aborts==1&&!firmwareWriter.owner);
 reset();ManualHandler third;AsyncWebServerRequest malformed;third.finish(&malformed);assert(malformed.status==500&&!g_pendingRestart);
 std::cout<<"PASS failure/disconnect/pending-restart: failed sessions release; rejected disconnect cannot abort owner; committed image stays reserved\n";
}
// WebRequest::_onData retains a shared request reference while callbacks run.
// Model synchronous close by invoking the real registered callback from send,
// and destroy the mock request only after the handler returns.
static void synchronousDisconnectLifecycle(){
 for(bool mid : {false,true}) {
  reset();ManualHandler h;const unsigned before=contextsFreed;
  {
   AsyncWebServerRequest r;h.chunk(r,mid ? "A" : "",0,false);
   assert(r._tempObject && Update.active && firmwareWriter.owner==&r);
   GitHubOtaUpdater u("o","r","2.0.3","board");prepare(u);u._state=GitHubOtaUpdater::State::UpdateAvailable;
   assert(u.startUpdate() && tasks.size()==1);tasks[0]->fn(tasks[0]->param);tasks.clear();
   assert(u._state==GitHubOtaUpdater::State::Error && Update.active && firmwareWriter.owner==&r);
   assert(Update.aborts==0 && Update.begins==1);dispose(u);
   r.disconnect();r.disconnect();
   assert(!Update.active && Update.aborts==1 && !firmwareWriter.owner && !g_pendingRestart);
   assert(contextsFreed==before);
  }
  assert(contextsFreed==before+1);
 }
 reset();ManualHandler h;const unsigned before=contextsFreed;
 {
  AsyncWebServerRequest r;h.chunk(r,"AB",0,true);r.disconnectOnSend=true;h.finish(&r);
  assert(r.status==200 && r.disconnectedOnce && firmwareWriter.committed && firmwareWriter.owner==&r);
  assert(Update.committed=="AB" && Update.aborts==0 && g_pendingRestart);
  assert(contextsFreed==before);
  GitHubOtaUpdater u("o","r","2.0.3","board");prepare(u);u._state=GitHubOtaUpdater::State::UpdateAvailable;
  assert(u.startUpdate() && tasks.size()==1);tasks[0]->fn(tasks[0]->param);tasks.clear();
  assert(u._state==GitHubOtaUpdater::State::Error && Update.begins==1 && Update.aborts==0);dispose(u);
 }
 assert(contextsFreed==before+1);
 reset();ManualHandler failed;const unsigned beforeFailure=contextsFreed;
 {
  AsyncWebServerRequest r;h.chunk(r,"A",0,true);Update.endFail=true;r.disconnectOnSend=true;failed.finish(&r);
  assert(r.status==500 && r.disconnectedOnce && Update.aborts==1 && !firmwareWriter.owner && !g_pendingRestart);
  assert(contextsFreed==beforeFailure);
 }
 assert(contextsFreed==beforeFailure+1);
 std::cout<<"PASS sync disconnect: start/mid abort once; end send preserves commit; context freed once; GitHub blocked\n";
}
static void githubCleanup(){
 for(int phase=0;phase<4;++phase){
  reset();GitHubOtaUpdater u("o","r","2.0.3","board");prepare(u);
  Update.allocationFail=phase==0;Update.writeFail=phase==1;Update.endFail=phase==2;
  if(phase==3)ota.failAt=1;
  u.doDownload();assert(u._state==GitHubOtaUpdater::State::Error&&!Update.active&&!firmwareWriter.owner&&!firmwareWriter.committed&&!u._restartPending);
  Update.allocationFail=Update.writeFail=Update.endFail=false;
  ManualHandler h;AsyncWebServerRequest next;h.chunk(next,"M");h.finish(&next);assert(next.status==200&&Update.committed=="M");dispose(u);
 }
 std::cout<<"PASS GitHub cleanup: begin/write/finalize/incomplete failures release ownership for next manual request\n";
}
#endif
static void multiFile(){ reset();ManualHandler h;AsyncWebServerRequest r;h.chunk(r,"A",0,true);h.chunk(r,"B",0,true);h.finish(&r);assert(r.status==500&&!Update.active&&Update.committed.empty()&&!g_pendingRestart);std::cout<<"PASS multiple files rejected without commit\n";}
int main(){firmwareWriter.begin();static_assert(!std::is_copy_constructible<FirmwareWriterLease>::value, "lease must not be copied"); assert(!firmwareWriter.acquire(nullptr)); githubFirst();manualFirst();twoManuals();beginFailure();disconnect();unauthorized();multiFile();
#if FIXED
 completionAndFailure();synchronousDisconnectLifecycle();githubCleanup();
#endif
}
