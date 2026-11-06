#include <Arduino.h>
#include <cassert>
#include <atomic>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <thread>
#include <freertos/semphr.h>
uint32_t testNow=100;FakeEsp ESP;SnapshotStringFaults outputFault;
struct TrackingAllocator:ArduinoJson::Allocator {
 size_t calls=0,live=0,peak=0,failAt=SIZE_MAX;std::map<void*,size_t> blocks;
 void*allocate(size_t n)override{if(calls++==failAt)return nullptr;void*p=malloc(n);if(p){blocks[p]=n;live+=n;peak=std::max(peak,live);}return p;}
 void deallocate(void*p)override{if(p){live-=blocks.at(p);blocks.erase(p);free(p);}}
 void*reallocate(void*p,size_t n)override{if(calls++==failAt)return nullptr;const size_t old=p?blocks.at(p):0;const uintptr_t oldAddress=reinterpret_cast<uintptr_t>(p);void*q=realloc(p,n);if(q){if(oldAddress)blocks.erase(reinterpret_cast<void*>(oldAddress));blocks[q]=n;live=live-old+n;peak=std::max(peak,live);}return q;}
} allocator;
using RealJsonDocument=ArduinoJson::JsonDocument;
class TracedJsonDocument:public RealJsonDocument{public:explicit TracedJsonDocument(ArduinoJson::Allocator*a=&::allocator):RealJsonDocument(a){}};
#define JsonDocument TracedJsonDocument
#define private public
#include "core/SolarState.h"
#undef private
JsonDocument g_stateDoc;JsonObject deviceJson,staticData,liveData;
#include "production-snapshot.inc"
static std::string canonical(JsonVariantConst v){std::string s;serializeJson(v,s);return s;}
static void load(const char*file,RealJsonDocument&fixture){std::ifstream f(file);const std::string text((std::istreambuf_iterator<char>(f)),{});assert(!deserializeJson(fixture,text));}
static void install(SolarState&s,const RealJsonDocument&f){g_stateDoc.clear();assert(g_stateDoc.set(f["data"]));assert(s.commitInverterState());assert(s.commitLocalState());}
static void unlocked(SolarState&s){assert(s._snapshotMutex->try_lock());s._snapshotMutex->unlock();}
static void equalToClone(SolarState&s){JsonDocument expected;assert(s.snapshotTo(expected));String actual;assert(s.serializeSnapshot(actual));assert(actual.text==canonical(expected));unlocked(s);}
static void fixtureCase(const RealJsonDocument&fixture,const char*name){
 SolarState s;s._snapshotMutex=xSemaphoreCreateMutex();install(s,fixture);
 const size_t permanent=allocator.live;allocator.peak=permanent;size_t cloneCalls=0;
 {JsonDocument clone;const size_t calls=allocator.calls;assert(s.snapshotTo(clone));cloneCalls=allocator.calls-calls;assert(canonical(clone)==fixture["expectedData"].as<const char*>());}
 const size_t clonePeak=allocator.peak-permanent;assert(clonePeak>1000&&cloneCalls>0&&allocator.live==permanent);
 allocator.peak=permanent;const size_t before=allocator.calls;allocator.failAt=before;String output;assert(s.serializeSnapshot(output));allocator.failAt=SIZE_MAX;
 assert(allocator.calls==before&&allocator.peak==permanent&&output.text==fixture["expectedData"].as<const char*>()&&outputFault.reserves==1);unlocked(s);
 std::cout<<"{\"fixture\":\""<<name<<"\",\"dataBytes\":"<<output.length()<<",\"baselineClonePeakJsonBytes\":"<<clonePeak<<",\"baselineCloneAllocations\":"<<cloneCalls<<",\"newReaderJsonAllocations\":0,\"newReaderPeakJsonBytes\":0}\n";
 for(int kind=0;kind<4;kind++){outputFault=SnapshotStringFaults{};outputFault.reserveFailure=kind==0;outputFault.writeBudget=kind==1?0:kind==2?1:output.length()/2;String partial("old");assert(!s.serializeSnapshot(partial));assert(partial.length()==0);unlocked(s);}
 outputFault=SnapshotStringFaults{};
 s._inverterSnapshot["RawData"].as<JsonObject>().remove("CommandAnswer");s._localSnapshot.remove("CommandAnswer");equalToClone(s);
 s._inverterSnapshot["RawData"]["CommandAnswer"]="old";s._localSnapshot["CommandAnswer"]=nullptr;equalToClone(s);
 s._localSnapshot["CommandAnswer"]="new\n\"\\\t\xC3\xA4";s._localSnapshot["LocalData"]["Battery_Charge_Energy"]=123.5;equalToClone(s);
 s._localSnapshot["LocalData"].as<JsonObject>().remove("Battery_Charge_Energy");equalToClone(s);
 // Exercise position-preserving overwrite even outside the producer's disjoint-key invariant.
 s._localSnapshot["LocalData"]["Battery_Voltage"]=nullptr;s._localSnapshot["LocalData"]["extra\"\\\n"]="\"\\\n\xE2\x82\xAC";equalToClone(s);
 const char nulKey[]={'n','u','l','\0','k','e','y'};s._inverterSnapshot["LiveData"][JsonString(nulKey,sizeof(nulKey))]="nul key";equalToClone(s);
 const char nulValue[]={'a','\0','b'};s._localSnapshot["CommandAnswer"].set(JsonString(nulValue,sizeof(nulValue)));equalToClone(s);
 const size_t calls=allocator.calls;String control;assert(s.serializeSnapshot(control));const size_t keyAllocations=allocator.calls-calls;assert(keyAllocations>0);
 for(size_t fail=0;fail<keyAllocations;fail++){allocator.failAt=allocator.calls+fail;String failed("stale");assert(!s.serializeSnapshot(failed)&&failed.length()==0);allocator.failAt=SIZE_MAX;unlocked(s);}
 s._inverterSnapshot.clear();s._localSnapshot.clear();equalToClone(s);vSemaphoreDelete(s._snapshotMutex);s._snapshotMutex=nullptr;
 String missing("stale");assert(!s.serializeSnapshot(missing)&&missing.length()==0);outputFault=SnapshotStringFaults{};
}
static void concurrentCommit(const RealJsonDocument&first,const RealJsonDocument&second){
 SolarState s;s._snapshotMutex=xSemaphoreCreateMutex();install(s,first);JsonDocument expected;assert(s.snapshotTo(expected));const std::string original=canonical(expected);std::atomic<bool>waiting{false},committed{false};std::thread producer;
 outputFault.onReserve=[&]{producer=std::thread([&]{g_stateDoc.clear();assert(g_stateDoc.set(second["data"]));assert(!s._snapshotMutex->try_lock());waiting=true;assert(s.commitInverterState());assert(s.commitLocalState());committed=true;});while(!waiting.load())std::this_thread::yield();assert(!committed.load());};
 String output;assert(s.serializeSnapshot(output));outputFault=SnapshotStringFaults{};producer.join();assert(output.text==original&&committed.load());equalToClone(s);unlocked(s);vSemaphoreDelete(s._snapshotMutex);s._snapshotMutex=nullptr;
}
int main(int argc,char**argv){assert(argc==3);RealJsonDocument gk,max;load(argv[1],gk);load(argv[2],max);fixtureCase(gk,"gk");fixtureCase(max,"max");concurrentCommit(gk,max);concurrentCommit(max,gk);std::puts("PASS exact fixtures, overlays/null/NUL/escaping, no reader clone, OOM/short writes/unlock, concurrent commits");}
