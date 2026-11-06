#define MODE 2
#include <Arduino.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <vector>
#include <freertos/semphr.h>
#include <thread>
#include <condition_variable>
#include <chrono>
#define private public
#include "core/GitHubOtaUpdater.h"
#undef private
#include "production-ota-guard.inc"

// Fail the real std::vector allocation inside extracted library code by its
// exact payload size. Ordinary application Strings allocate length+1 instead.
static bool failVectorAllocation=false;
static size_t failVectorSize=0;
void *operator new(size_t size){if(failVectorAllocation&&size==failVectorSize)throw std::bad_alloc();void *p=std::malloc(size);if(!p)throw std::bad_alloc();return p;}
void operator delete(void *p)noexcept{std::free(p);}
void *operator new[](size_t n){return ::operator new(n);}
void operator delete[](void *p)noexcept{::operator delete(p);}
void operator delete(void *p,size_t)noexcept{::operator delete(p);}
void operator delete[](void *p,size_t)noexcept{::operator delete(p);}
void *operator new(size_t size,const std::nothrow_t &)noexcept{if(failVectorAllocation&&size==failVectorSize)return nullptr;return std::malloc(size);}
void operator delete(void *p,const std::nothrow_t &)noexcept{std::free(p);}

uint32_t testNow=100;FakeEsp ESP;
StringFaultState stringFault;
struct TrackingAllocator:ArduinoJson::Allocator {
 std::map<void *,size_t> blocks;size_t live=0,peak=0,attempts=0,failAt=SIZE_MAX;
 bool fail(){return attempts++==failAt;}
 void *allocate(size_t size)override{if(fail())return nullptr;void *p=std::malloc(size);if(p){blocks[p]=size;live+=size;peak=std::max(peak,live);}return p;}
 void deallocate(void *p)override{if(p){auto it=blocks.find(p);assert(it!=blocks.end());live-=it->second;blocks.erase(it);std::free(p);}}
 void *reallocate(void *p,size_t size)override{
  if(fail())return nullptr;
  size_t old=p?blocks.at(p):0;
  void *q=std::realloc(p,size);
  if(q){if(p)blocks.erase(p);blocks[q]=size;live=live-old+size;peak=std::max(peak,live);}return q;
 }
} tracking;
// Same real ArduinoJson implementation and pool sizes, with only the allocator
// constructor selected for accounting and deterministic one-allocation faults.
using RealJsonDocument=ArduinoJson::JsonDocument;
class TracedJsonDocument:public RealJsonDocument {
public:TracedJsonDocument():ArduinoJson::JsonDocument(&tracking){}
};
#define JsonDocument TracedJsonDocument
static size_t permanentJsonBytes=0;
static size_t transientBytes(){assert(tracking.live>=permanentJsonBytes);return tracking.live-permanentJsonBytes;}
static const void *observedCache=nullptr;static size_t *cacheLiveCounter=nullptr;
void observeStringReserve(const void *value){if(value==observedCache&&cacheLiveCounter)*cacheLiveCounter=transientBytes();}
class SnapshotLock{public:explicit SnapshotLock(void *){}};
class SolarState{
public:JsonDocument _inverterSnapshot,_localSnapshot;void *_snapshotMutex=xSemaphoreCreateMutex();bool snapshotTo(JsonDocument &target);bool serializeSnapshot(String &output);
 ~SolarState(){vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_snapshotMutex));}
};
using AsyncWebSocketSharedBuffer=std::shared_ptr<std::vector<uint8_t>>;
enum AwsEventType { WS_EVT_CONNECT };
class AsyncWebSocket;
struct AsyncWebSocketClient {unsigned messages=0;std::string last;void text(const String &p);};
class AsyncWebSocket {
public:
 enum SendStatus{DISCARDED,ENQUEUED,PARTIALLY_ENQUEUED};
 size_t clients=0,calls=0,allocatedBytes=0,jsonLiveAtBroadcast=0;std::string lastPayload;bool queueFull=false;
 std::function<void(AsyncWebSocket *,AsyncWebSocketClient *,AwsEventType,void *,uint8_t *,size_t)> connected;
 void onEvent(decltype(connected) fn){connected=std::move(fn);}
 bool availableForWriteAll(){return !queueFull;}
 size_t count()const{return clients;}
 SendStatus textAll(const uint8_t *,size_t);SendStatus textAll(const char *,size_t);SendStatus textAll(const String &);
 SendStatus textAll(AsyncWebSocketSharedBuffer buffer){++calls;allocatedBytes+=buffer->size();jsonLiveAtBroadcast=transientBytes();lastPayload.assign(buffer->begin(),buffer->end());return clients?ENQUEUED:DISCARDED;}
};
#include "production-websocket.inc"
void AsyncWebSocketClient::text(const String &payload)
{
    // Match the library's payload-buffer allocation for a connected client.
    auto buffer = makeSharedBuffer(reinterpret_cast<const uint8_t *>(payload.c_str()), payload.length());
    ++messages;
    last.assign(buffer->begin(), buffer->end());
}
static bool responseCopyFailure=false;static unsigned responseDeletes=0;
struct AsyncWebServerRequest;
class AsyncBasicResponse {
protected:size_t _contentLength=0;
private:String _content;int _code;friend struct AsyncWebServerRequest;
public:
 AsyncBasicResponse(int code,const char *,const char *content):_code(code){
  const size_t previous=stringFault.copyFailureSize;
  if(responseCopyFailure)stringFault.copyFailureSize=std::strlen(content);
  _content=String(content);stringFault.copyFailureSize=previous;
  if(_content.length())_contentLength=_content.length();
 }
 virtual ~AsyncBasicResponse(){++responseDeletes;}
};
struct AsyncWebServerRequest{
 bool authorized=true;int status=0;String body;size_t jsonLiveAtSend=0;
 void requestAuthentication(){status=401;}
 void send(int code,const char *,const String &payload){status=code;jsonLiveAtSend=transientBytes();body.text=payload.text;}
 void send(AsyncBasicResponse *response){status=response->_code;jsonLiveAtSend=transientBytes();body.text=response->_content.text;delete response;}
};
constexpr int HTTP_GET=1;
struct FakeServer{
 std::function<void(AsyncWebServerRequest *)> data,status;
 void on(const char *route,int,std::function<void(AsyncWebServerRequest *)> fn){if(std::string(route)=="/api/data")data=std::move(fn);else{assert(std::string(route)=="/api/status");status=std::move(fn);}}
 void addHandler(AsyncWebSocket *){}
};
struct FakeWifi{
 String address,host;bool connected=true;
 bool getConnectionState()const{return connected;}bool isEthActive()const{return false;}bool isInApMode()const{return false;}
 int rssi()const{return -56;}String ipAddress()const{return address;}String hostName()const{return host;}
};
struct FakeInverter{
 String name;String protocolName()const{return name;}bool simulationEnabled()const{return false;}
 bool loopbackRunning()const{return false;}bool loopbackDone()const{return false;}bool loopbackOk()const{return false;}
 String loopbackMessage()const{return "";}
};
struct FakeSettings{
 String name;FakeSettings &get=*this;const char *deviceName()const{return name.c_str();}
} _settings;
#define BUILD_VARIANT "wemos_d1_mini32"
class WebServerHandler{
public:
 SolarState _state;AsyncWebSocket _wsStatus;FakeServer _server;FakeWifi _wifiManager;FakeInverter _inverterService;
 GitHubOtaUpdater _otaUpdater{"owner","repo","2.0.3","board"};
 WebServerHandler(){_otaUpdater._lockHandle=xSemaphoreCreateMutex();}
 ~WebServerHandler(){vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_otaUpdater._lockHandle));}
 bool _mqttConnected=true,_inverterConnected=true;String _lastStatusPayload;String &cached=_lastStatusPayload;size_t jsonLiveAtCache=0;SemaphoreHandle_t _statusPayloadMutex=nullptr;
 bool isAuthorized(AsyncWebServerRequest *request){return request->authorized;}
 bool buildStatusJson(JsonDocument &doc);
 void refreshStatusPayload();void registerDataRoute();void setupStatusWebSocket();bool copyLastStatusPayload(String &payload);
 void setLastStatusPayload(const String &payload);
};
#include "production-overflow.inc"
// The actual guard and status/WebSocket functions are extracted from production.
static void statusGuardBehavior(WebServerHandler &h, const std::string &status)
{
    h._wsStatus.clients = 1;
    h.setupStatusWebSocket();
    const auto checkSkipped = [&]() {
        const size_t calls = h._wsStatus.calls;
        const size_t jsonAllocations = tracking.attempts;
        const auto previous = h.cached.text;
        AsyncWebSocketClient initial;
        failVectorSize = status.size();
        failVectorAllocation = true;
        h.refreshStatusPayload();
        h._wsStatus.connected(&h._wsStatus, &initial, WS_EVT_CONNECT, nullptr, nullptr, 0);
        failVectorAllocation = false;
        assert(h._wsStatus.calls == calls && initial.messages == 0);
        assert(tracking.attempts == jsonAllocations && h.cached.text == previous);
    };
    for (int reason = 0; reason < 4; ++reason)
    {
        h._otaUpdater._state = reason == 0 ? GitHubOtaUpdater::State::Checking :
                               reason == 1 ? GitHubOtaUpdater::State::Downloading : GitHubOtaUpdater::State::Idle;
        h._otaUpdater._task = reason == 2 ? reinterpret_cast<void *>(1) : nullptr;
        h._otaUpdater._restartPending = reason == 3;
        checkSkipped();
    }
    h._otaUpdater._state = GitHubOtaUpdater::State::Idle;
    h._otaUpdater._task = nullptr;
    h._otaUpdater._restartPending = false;

    // The websocket callback can already hold a library lock. A contended OTA
    // mutex must reject it promptly rather than wait and invert the lock order.
    auto *mutex = static_cast<SemaphoreHandle_t>(h._otaUpdater._lockHandle);
    mutex->lock();
    std::mutex sync;
    std::condition_variable cv;
    bool done = false;
    std::thread attempt([&] {
        checkSkipped();
        AsyncWebServerRequest data, cachedStatus;
        h._server.data(&data);
        h._server.status(&cachedStatus);
        assert(data.status == 200 && cachedStatus.status == 200 && cachedStatus.body.text == status);
        { std::lock_guard<std::mutex> lock(sync); done = true; }
        cv.notify_one();
    });
    bool finishedWhileLocked;
    {
        std::unique_lock<std::mutex> lock(sync);
        finishedWhileLocked = cv.wait_for(lock, std::chrono::milliseconds(250), [&] { return done; });
    }
    mutex->unlock();
    attempt.join();
    assert(finishedWhileLocked);

    // The guard affects websocket work only; regular HTTP data/status remain usable.
    h._otaUpdater._state = GitHubOtaUpdater::State::Checking;
    AsyncWebServerRequest data, cachedStatus;
    h._server.data(&data);
    h._server.status(&cachedStatus);
    assert(data.status == 200 && cachedStatus.status == 200 && cachedStatus.body.text == status);
    h._otaUpdater._state = GitHubOtaUpdater::State::Idle;
    const size_t previousCalls = h._wsStatus.calls;
    h.refreshStatusPayload();
    assert(h._wsStatus.calls == previousCalls + 1 && h._wsStatus.lastPayload == status);
    AsyncWebSocketClient initial;
    h._wsStatus.connected(&h._wsStatus, &initial, WS_EVT_CONNECT, nullptr, nullptr, 0);
    assert(initial.messages == 1 && initial.last == status);
    h._wsStatus.clients = 0;
    std::puts("PASS: status guard skips busy/contended websocket work, keeps HTTP and resumes when idle.");
}

static const char *modeName(){return MODE==0?"before":MODE==1?"memory":"overflow";}
static bool exercise(const std::string &fixture,int id){
 assert(tracking.live==0);std::ifstream input(fixture);std::string text((std::istreambuf_iterator<char>(input)),{});
 RealJsonDocument original;assert(!deserializeJson(original,text));
 WebServerHandler h;auto data=original["data"].as<JsonObjectConst>();
 for(const char *section:{"RawData","DeviceData","LiveData"})h._state._inverterSnapshot[section]=data[section];
 for(const char *section:{"EspData","Status"})h._state._localSnapshot[section]=data[section];
 h._state._localSnapshot["CommandAnswer"]=data["RawData"]["CommandAnswer"];
 h._wifiManager.address=original["status"]["ip"].as<const char *>();h._wifiManager.host=original["status"]["hostName"].as<const char *>();
 h._inverterService.name=original["status"]["protocol"].as<const char *>();_settings.name=data["EspData"]["Device_name"].as<const char *>();
 permanentJsonBytes=tracking.live;assert(permanentJsonBytes>0);h.registerDataRoute();observedCache=&h._lastStatusPayload;cacheLiveCounter=&h.jsonLiveAtCache;
 const size_t normalStart=tracking.attempts;h.refreshStatusPayload();const std::string status=h.cached.text;
 const size_t normalAllocations=tracking.attempts-normalStart;
 const size_t noClientCalls=h._wsStatus.calls,statusCacheLive=h.jsonLiveAtCache,noClientBytes=h._wsStatus.allocatedBytes;
 assert(!status.empty());assert(original["expectedStatus"].is<const char *>());assert(status==original["expectedStatus"].as<const char *>());
 statusGuardBehavior(h,status);
 // Empty subscriber list must not trigger a throwing payload-sized allocation.
 failVectorSize=status.size();failVectorAllocation=true;bool noClientThrows=false;
 try{h.refreshStatusPayload();}catch(const std::bad_alloc &){noClientThrows=true;}failVectorAllocation=false;
 assert(h.cached.text==status);
 assert(noClientCalls==0&&noClientBytes==0&&!noClientThrows&&statusCacheLive==0);
 h._wsStatus.clients=1;h.refreshStatusPayload();assert(h._wsStatus.lastPayload==status);
 const size_t connectedBroadcastLive=h._wsStatus.jsonLiveAtBroadcast;
 assert(connectedBroadcastLive==0);
 h._wsStatus.clients=0;
 AsyncWebServerRequest request;h._server.data(&request);assert(request.status==200);
 const std::string dataPayload=request.body.text;const size_t dataSendLive=request.jsonLiveAtSend;
 RealJsonDocument parsed;assert(!deserializeJson(parsed,dataPayload));assert(parsed["DeviceData"]==data["DeviceData"]);assert(parsed["LiveData"]==data["LiveData"]);
 assert(original["expectedData"].is<const char *>());assert(dataPayload==original["expectedData"].as<const char *>());
 assert(dataSendLive==0);
 // Sweep real allocation positions: RawData-only failure need not change status,
 // since buildStatusJson intentionally leaves that section out of its payload.
 bool partialStatusPublished=false;size_t firstPartialStatusFault=SIZE_MAX;
 for(size_t fault=0;fault<normalAllocations;++fault){
  h.cached=status;tracking.failAt=tracking.attempts+fault;h.refreshStatusPayload();tracking.failAt=SIZE_MAX;
  if(h.cached.text!=status){partialStatusPublished=true;firstPartialStatusFault=fault;break;}
 }
 const size_t attemptsBeforeData=tracking.attempts;
 tracking.failAt=tracking.attempts;AsyncWebServerRequest failed;h._server.data(&failed);tracking.failAt=SIZE_MAX;
 const bool partialDataSent=failed.status==200&&failed.body.text!=dataPayload;
 assert(!partialStatusPublished&&!partialDataSent&&failed.status==200&&failed.body.text==dataPayload);
 assert(tracking.attempts==attemptsBeforeData); // No transient ArduinoJson reader clone.
 bool serializationFaultRejected=true;
 for(int kind=0;kind<2;++kind){
  h.cached=status;h._wsStatus.clients=1;const size_t broadcasts=h._wsStatus.calls;
  stringFault.reserveFailure=kind==0;stringFault.writeBudget=kind==1?status.size()/2:SIZE_MAX;
  h.refreshStatusPayload();stringFault=StringFaultState{};
  const bool keptCache=h.cached.text==status&&h._wsStatus.calls==broadcasts;
  stringFault.reserveFailure=kind==0;stringFault.writeBudget=kind==1?dataPayload.size()/2:SIZE_MAX;
  AsyncWebServerRequest serializationFailed;h._server.data(&serializationFailed);stringFault=StringFaultState{};
  serializationFaultRejected=serializationFaultRejected&&keptCache&&serializationFailed.status==503;
  assert(keptCache&&serializationFailed.status==503&&serializationFailed.jsonLiveAtSend==0);
 }
 // A saturated queue must not allocate another vector or close slow clients.
 h.cached=status;h._wsStatus.clients=1;h._wsStatus.queueFull=true;const size_t queued=h._wsStatus.calls;
 failVectorSize=status.size();failVectorAllocation=true;h.refreshStatusPayload();failVectorAllocation=false;
 assert(h._wsStatus.calls==queued&&h.cached.text==status);h._wsStatus.queueFull=false;
 // Validate the second String copy performed by the real response constructor.
 for(bool cachedRoute:{false,true}){
  AsyncWebServerRequest copyFailed;responseCopyFailure=true;const unsigned deleted=responseDeletes;
  (cachedRoute?h._server.status:h._server.data)(&copyFailed);responseCopyFailure=false;
  assert(copyFailed.status==503&&copyFailed.jsonLiveAtSend==0&&responseDeletes==deleted+1);
 }
 AsyncWebServerRequest objectFailed;failVectorSize=sizeof(CheckedJsonResponse);failVectorAllocation=true;
 sendCompleteJson(&objectFailed,String(status));failVectorAllocation=false;assert(objectFailed.status==503);
 // Explicit cache-copy failure is distinct from an empty startup cache.
 for(bool locked:{false,true}){
  if(locked)h._statusPayloadMutex=xSemaphoreCreateMutex();
  String copied;assert(h.copyLastStatusPayload(copied)&&copied.text==status);
  stringFault.copyFailureSize=status.size();AsyncWebServerRequest cacheFailed;h._server.status(&cacheFailed);stringFault=StringFaultState{};
  assert(cacheFailed.status==503&&h.cached.text==status);
  if(locked){vSemaphoreDelete(h._statusPayloadMutex);h._statusPayloadMutex=nullptr;}
 }
 h.setupStatusWebSocket();AsyncWebSocketClient initial;
 stringFault.copyFailureSize=status.size();h._wsStatus.connected(&h._wsStatus,&initial,WS_EVT_CONNECT,nullptr,nullptr,0);stringFault=StringFaultState{};
 assert(initial.messages==0);h._wsStatus.connected(&h._wsStatus,&initial,WS_EVT_CONNECT,nullptr,nullptr,0);assert(initial.messages==1&&initial.last==status);
 AsyncWebServerRequest statusGood;h._server.status(&statusGood);assert(statusGood.status==200&&statusGood.body.text==status&&statusGood.jsonLiveAtSend==0);
 h.cached="";String empty;assert(h.copyLastStatusPayload(empty)&&empty.length()==0);
 AsyncWebServerRequest startup;h._server.status(&startup);assert(startup.status==200&&startup.body.length()>0&&startup.jsonLiveAtSend==0);
 tracking.failAt=tracking.attempts;AsyncWebServerRequest startupFailed;h._server.status(&startupFailed);tracking.failAt=SIZE_MAX;assert(startupFailed.status==503&&startupFailed.jsonLiveAtSend==0);
 h.cached=status;
 // reserve() failure preserves the old buffer; do not inject impossible copy
 // failure after a successful same-length reservation in the actual setter.
 for(bool locked:{false,true}){
  if(locked)h._statusPayloadMutex=xSemaphoreCreateMutex();
  const std::string previous=h.cached.text;
  const String larger(previous+std::string(h.cached.text.capacity()+1,' '));
  stringFault.reserveFailure=true;h.setLastStatusPayload(larger);stringFault=StringFaultState{};
  assert(h.cached.text==previous);
  if(locked){assert(h._statusPayloadMutex->try_lock());h._statusPayloadMutex->unlock();vSemaphoreDelete(h._statusPayloadMutex);h._statusPayloadMutex=nullptr;}
 }
 AsyncWebServerRequest unauthorized;unauthorized.authorized=false;h._server.data(&unauthorized);assert(unauthorized.status==401&&unauthorized.body.length()==0);
 std::cout<<"{\"mode\":\""<<modeName()<<"\",\"id\":"<<id<<",\"pointerBytes\":"<<sizeof(void *)
 <<",\"persistentSnapshotJsonBytes\":"<<permanentJsonBytes<<",\"statusBytes\":"<<status.size()<<",\"dataBytes\":"<<dataPayload.size()
 <<",\"noClientWsAllocations\":"<<noClientCalls<<",\"noClientWsBytes\":"<<noClientBytes<<",\"noClientAllocationThrows\":"<<(noClientThrows?"true":"false")
 <<",\"transientJsonBytesAtCacheCopy\":"<<statusCacheLive<<",\"transientJsonBytesAtConnectedBroadcast\":"<<connectedBroadcastLive<<",\"transientJsonBytesAtDataSend\":"<<dataSendLive
 <<",\"normalRefreshJsonAllocationCalls\":"<<normalAllocations<<",\"firstPartialStatusFaultIndex\":"<<(firstPartialStatusFault==SIZE_MAX?-1:static_cast<int>(firstPartialStatusFault))
 <<",\"faultPublishedPartialStatus\":"<<(partialStatusPublished?"true":"false")<<",\"faultSentPartialData200\":"<<(partialDataSent?"true":"false")
 <<",\"responseAndCacheCopyFailuresRejected\":true,\"fullWebsocketQueueSkipped\":true"
 <<",\"reserveAndShortSerializationRejected\":"<<(serializationFaultRejected?"true":"false")<<"}\n";
 return noClientCalls==0&&!noClientThrows&&statusCacheLive==0&&connectedBroadcastLive==0&&dataSendLive==0;
}
int main(int argc,char **argv){assert(argc>=3);bool safe=true;for(int i=1;i<=2;++i){safe=exercise(argv[i],i==1?24:25)&&safe;assert(tracking.live==0);permanentJsonBytes=0;}
 if(argc>3&&!safe){std::cerr<<"Memory contract failed: unnecessary WS allocation or live transient JSON at duplication boundary\n";return 2;}
}
