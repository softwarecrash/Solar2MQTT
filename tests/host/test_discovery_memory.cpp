#include <cstdlib>
#include <new>
#include <set>
#include <fstream>
#include <iterator>
#include <string>
static bool failDiscoveryAllocation=false;
static size_t deniedDiscoverySize=0,deniedDiscoveryHits=0;
static size_t collectorCalls=0,incrementalCollectorCapacity=0;
void *operator new(size_t n){if(failDiscoveryAllocation&&n==deniedDiscoverySize){++deniedDiscoveryHits;throw std::bad_alloc();}void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}
void operator delete(void*p)noexcept{std::free(p);}
void *operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
void *operator new(size_t n,const std::nothrow_t&)noexcept{if(failDiscoveryAllocation&&n==deniedDiscoverySize){++deniedDiscoveryHits;return nullptr;}return std::malloc(n);}
void operator delete(void*p,const std::nothrow_t&)noexcept{std::free(p);}
#define FULL_HA_DISCOVERY 1
#define DISCOVERY_MEMORY_TEST 1
#include "test_battery_api.cpp"
void observeDiscoveryCollector(bool force,const std::vector<String>&collector){assert(!force);++collectorCalls;incrementalCollectorCapacity=std::max(incrementalCollectorCapacity,collector.capacity());}

static void load(Fixture&f,const char*file){std::ifstream input(file);const std::string text((std::istreambuf_iterator<char>(input)),{});JsonDocument fixture;assert(!deserializeJson(fixture,text));f.state.doc().clear();assert(f.state.doc().set(fixture["data"]));assert(f.state.commitInverterState());assert(f.state.commitLocalState());}
static std::set<std::string> topics(const Fixture&f){std::set<std::string> result;for(const auto&t:f.mqtt._haDiscoveryTopics)assert(result.insert(t.text).second);return result;}
static std::string config(const Fixture&f,const String&topic){for(const auto&m:f.mqtt._mqtt.messages)if(m.topic==topic.text&&!m.value.empty())return m.value;return {};}
static void repeatedIncremental(const char*fixture){
 battery_cases::reset();Fixture f(PI30_PIP_GK);load(f,fixture);f.mqtt.publishHaDiscovery(true);assert(f.mqtt._haDiscoveryTopics.size()>20);
 const auto original=topics(f);const auto originalOrder=f.mqtt._haDiscoveryTopics;const auto capacity=f.mqtt._haDiscoveryTopics.capacity();
 deniedDiscoverySize=(f.mqtt._haDiscoveryTopics.size()+16)*sizeof(String);deniedDiscoveryHits=0;collectorCalls=incrementalCollectorCapacity=0;f.mqtt._mqtt.messages.clear();
 bool threw=false;failDiscoveryAllocation=true;
 try{for(unsigned i=0;i<6;i++)f.mqtt.publishHaDiscovery(false);}catch(const std::bad_alloc&){threw=true;}
 failDiscoveryAllocation=false;
 if(threw){std::fprintf(stderr,"REPRO baseline runtime bad_alloc: rejected %zu bytes (%zu String slots), hits=%zu\n",deniedDiscoverySize,deniedDiscoverySize/sizeof(String),deniedDiscoveryHits);}
 assert(!threw&&deniedDiscoveryHits==0&&f.mqtt._mqtt.messages.empty());assert(topics(f)==original&&f.mqtt._haDiscoveryTopics.capacity()==capacity);
 assert(collectorCalls==6&&incrementalCollectorCapacity==0);
 // A late sensor remains discoverable through the unchanged incremental path.
 f.state.doc()["LiveData"]["DS18B20_late"]=21.5;assert(f.state.commitLocalState());f.mqtt.publishHaDiscovery(false);
 const String late=buildDiscoveryTopic(getHaDeviceId(),"sensor","DS18B20_late");const auto lateConfig=config(f,late);assert(!lateConfig.empty()&&f.mqtt.hasHaDiscoveryTopic(late));
 JsonDocument decoded;assert(!deserializeJson(decoded,lateConfig));assert(decoded["state_topic"]=="solar/test/LiveData/DS18B20_late");assert(decoded["unique_id"]==buildUniqueId(getHaDeviceId(),"EspData","DS18B20_late").c_str());
 f.mqtt._mqtt.messages.clear();f.mqtt.publishHaDiscovery(false);assert(f.mqtt._mqtt.messages.empty());
 // Forced reconciliation still deletes only stale configs and keeps active IDs/order.
 const String stale="homeassistant/sensor/test-board/stale/config";f.mqtt._haDiscoveryTopics.push_back(stale);
 f.state.doc()["LiveData"].as<JsonObject>().remove("DS18B20_late");assert(f.state.commitLocalState());f.mqtt.publishHaDiscovery(true);
 bool removedLate=false,removedStale=false;for(const auto&m:f.mqtt._mqtt.messages){if(m.topic==late.text&&m.value.empty())removedLate=true;if(m.topic==stale.text&&m.value.empty())removedStale=true;}
 assert(removedLate&&removedStale&&!f.mqtt.hasHaDiscoveryTopic(late)&&!f.mqtt.hasHaDiscoveryTopic(stale));assert(topics(f)==original&&f.mqtt._haDiscoveryTopics==originalOrder);
 std::printf("PASS discovery: %zu active topics, denied former %zu-byte reserve unused across 6 incremental passes; late entity and forced cleanup unchanged\n",original.size(),deniedDiscoverySize);
}
static void settingControls(const char*fixture,protocol_type_t protocol){
 battery_cases::reset();Fixture f(protocol);load(f,fixture);
 // Reuse captured setting values with the two control-surface protocols.
 // This synthetic configuration flag exercises the command-switch helper;
 // it is not a new inverter frame or measurement.
 f.state.doc()["Status"]["protocol"]=protocolToString(protocol);f.state.doc()["DeviceData"][DESCR_Battery_Equalization_Enabled]=true;
 assert(f.state.commitInverterState());assert(f.state.commitLocalState());f.mqtt.publishHaDiscovery(true);
 const auto before=f.mqtt._haDiscoveryTopics;const auto device=getHaDeviceId();
 for(const auto&kind:std::vector<std::pair<const char*,const char*>>{{"select",DESCR_Output_Source_Priority},{"number",DESCR_Battery_Bulk_Voltage}}){const auto topic=buildDiscoveryTopic(device,kind.first,kind.second);assert(f.mqtt.hasHaDiscoveryTopic(topic)&&!config(f,topic).empty());}
 if(protocol==MODBUS_POWMR_PI){for(const char*key:{DESCR_Overload_Bypass_Enabled,DESCR_Battery_Equalization_Enabled}){const auto topic=buildDiscoveryTopic(device,"switch",key);assert(f.mqtt.hasHaDiscoveryTopic(topic)&&!config(f,topic).empty());}}
 f.mqtt._mqtt.messages.clear();deniedDiscoverySize=(before.size()+16)*sizeof(String);deniedDiscoveryHits=0;collectorCalls=incrementalCollectorCapacity=0;failDiscoveryAllocation=true;bool threw=false;
 try{for(unsigned i=0;i<3;i++)f.mqtt.publishHaDiscovery(false);}catch(const std::bad_alloc&){threw=true;}failDiscoveryAllocation=false;
 assert(!threw&&deniedDiscoveryHits==0&&f.mqtt._haDiscoveryTopics==before);
 assert(collectorCalls==3&&incrementalCollectorCapacity==0);
 // Hybrid's existing intentional passive/web-only cleanup remains unchanged.
 for(const auto&m:f.mqtt._mqtt.messages)assert(m.value.empty());
 f.mqtt._mqtt.messages.clear();f.mqtt.publishHaDiscovery(true);assert(f.mqtt._haDiscoveryTopics==before);
 std::printf("PASS actual %s control helpers: %s identities and forced order retained; no incremental collector\n",protocolToString(protocol),protocol==MODBUS_POWMR_PI?"select/number/switch/command-switch":"select/number");
}
static void pendingMigration(const char*fixture){
 battery_cases::reset(false);Fixture f(PI30_PIP_GK);load(f,fixture);f.mqtt._energyBacklog.begin();f.mqtt.initializeBatteryApi("test-board","test-boot");f.mqtt._energyBacklog.updateBatteryEnergy(f.state,BatterySampleTiming{},testNow);assert(f.mqtt._energyBacklog.batteryMigrationPending());f.mqtt.publishHaDiscovery(true);const auto before=f.mqtt._haDiscoveryTopics;
 collectorCalls=incrementalCollectorCapacity=0;f.mqtt._mqtt.messages.clear();f.mqtt.publishHaDiscovery(false);assert(collectorCalls==1&&incrementalCollectorCapacity==0&&f.mqtt._haDiscoveryTopics==before);
 for(const char*key:{DESCR_Battery_Charge_Energy,DESCR_Battery_Discharge_Energy}){const auto topic=buildDiscoveryTopic(getHaDeviceId(),"sensor",key);assert(f.mqtt.hasHaDiscoveryTopic(topic));for(const auto&m:f.mqtt._mqtt.messages)assert(m.topic!=topic.text);}
 std::puts("PASS pending migration: incremental pass allocates no collector, both retained battery reservations preserved");
}
int main(int argc,char**argv){assert(argc==3);repeatedIncremental(argv[1]);repeatedIncremental(argv[2]);settingControls(argv[1],MODBUS_POWMR);settingControls(argv[1],MODBUS_POWMR_PI);pendingMigration(argv[1]);battery_cases::apiAndMqtt();battery_cases::discoveryAfterMigration();std::puts("PASS actual complete HA publication helpers, pending/failed/verified migration topics and identities unchanged");}
