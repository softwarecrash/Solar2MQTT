// Only device setup/transport/OS are substituted. Scheduler, register decoding,
// cached acquisition timestamps and energy integration come from production.
module.exports=({write,read,extract})=>{
 write('freertos/FreeRTOS.h','#pragma once\n#include <Arduino.h>\n');
 write('freertos/task.h','#pragma once\nusing TaskHandle_t=void *;\n');
 write('modbus/powmr_hunt.h','#pragma once\nstruct PowmrHunt {};\n');
 write('solar/InverterHardware.h','#pragma once\nstruct TestHardware {int dirPin=-1;};\nextern TestHardware g_inverterHardwareConfig;\n');
 write('ModbusMaster.h',String.raw`#pragma once
#include <Arduino.h>
#include <map>
#include <set>
#include <vector>
struct WireRead {uint16_t start,count;uint32_t at;};
struct TestWire {
 std::map<uint16_t,uint16_t> words;std::set<uint16_t> failures;std::vector<WireRead> reads;
 uint32_t latencyMs=0;bool failAll=false;
};
extern TestWire wire;
class ModbusMaster {public:
 enum:uint8_t {ku8MBSuccess=0,ku8MBIllegalFunction=1,ku8MBIllegalDataAddress=2,ku8MBIllegalDataValue=3,ku8MBSlaveDeviceFailure=4,ku8MBInvalidSlaveID=0xe0,ku8MBInvalidFunction=0xe1,ku8MBResponseTimedOut=0xe2,ku8MBInvalidCRC=0xe3};
 uint16_t start=0,timeout=150;
 void setResponseTimeout(uint16_t n){timeout=n;}uint16_t getResponseTimeout(){return timeout;}
 void preTransmission(void(*)()){}void postTransmission(void(*)()){}
 uint8_t writeSingleRegister(uint16_t r,uint16_t v){wire.words[r]=v;return ku8MBSuccess;}
 uint8_t readHoldingRegisters(uint16_t r,uint16_t n){
   start=r;testNow+=wire.latencyMs;wire.reads.push_back({r,n,testNow});
   for(uint32_t i=r;i<uint32_t(r)+n;i++)if(wire.failAll||wire.failures.count(uint16_t(i)))return ku8MBResponseTimedOut;
   return ku8MBSuccess;
 }
 uint16_t getResponseBuffer(uint16_t i){return wire.words[uint16_t(start+i)];}
};
`);
 write('TestModbusDevice.h',String.raw`#pragma once
#include "modbus/modbus_com.h"
#include "solar/SolarTypes.h"
class ModbusDevice {public:
 const modbus_register_t *live=nullptr,*stat=nullptr;size_t liveCount=0,staticCount=0;protocol_type_t protocol=MODBUS_SMG;
 virtual ~ModbusDevice()=default;
 const modbus_register_t *getLiveRegisters()const{return live;}
 const modbus_register_t *getStaticRegisters()const{return stat;}
 size_t getLiveRegistersCount()const{return liveCount;}
 size_t getStaticRegistersCount()const{return staticCount;}
 protocol_type_t getProtocol()const{return protocol;}
};
extern uint32_t pv2DelayMs;
class PowMr:public ModbusDevice {public:void pollPv2(MODBUS_COM &,JsonObject){delay(pv2DelayMs);}};
`);
 const header=read('src/modbus/modbus.h').replace(/#include "device\/[^\n]+\n/g,'').replace('"modbus_com.h"','"modbus/modbus_com.h"');
 write('modbus/modbus.h','#include "TestModbusDevice.h"\n'+header);
 const driver=read('src/modbus/modbus.cpp');
 const methods=['MODBUS::MODBUS','MODBUS::~MODBUS','void MODBUS::prepareRegisters','void MODBUS::restartStateRead','void MODBUS::loop','void MODBUS::callback',
  'uint8_t MODBUS::batteryFieldMask','void MODBUS::beginBatteryPass','void MODBUS::noteBatteryRegister','void MODBUS::finishBatteryPass','void MODBUS::acceptBatterySample','BatterySampleTiming MODBUS::batterySampleTiming'];
 write('modbus_timing_generated.h',methods.map(s=>extract(driver,s)).join('\n'));
 let profiles='';
 for(const name of ['smg','smg_ii_11kw','must_pv_ph18','powmr','anenji','anenji_srne','deye']){
   const text=read(`src/modbus/device/${name}/${name}.h`);
   const match=text.match(/registers_live\[\]\s*=\s*(\{[\s\S]*?\n\s*\};)/);
   if(!match)throw Error('Missing live table '+name);
   const constants=[...text.matchAll(/static constexpr (?:uint16_t|const char \*)\s*\w+\s*=\s*[^;]+;/g)].map(m=>m[0]).join('\n');
   const defines=[...text.matchAll(/^#define \w+ "[^\n]+/gm)].map(m=>m[0]).join('\n');
   // These callbacks only extend nonbattery data; their separate behaviour is
   // outside this timing test. No register definitions or positions change.
   const table=match[1].replace(/(?:MustPV_PH18::generationSum|AnenjiSrne::extendedInverterData)/g,'ignoredNonBatteryCallback');
   profiles+=`${defines}\nnamespace profile_${name} {\n${constants}\nconst modbus_register_t live[]=${table}\n}\n`;
 }
 const smgStatic=read('src/modbus/device/smg/smg.h').match(/registers_static\[\]\s*=\s*(\{[\s\S]*?\n\s*\};)/);
 if(!smgStatic)throw Error('Missing SMG static recovery table');
 profiles+='\nnamespace profile_smg { const modbus_register_t restart_static[]='+smgStatic[1]+'\n}\n';
 write('modbus_profiles_generated.h',profiles);
 return {test:'test_modbus_battery.cpp',sources:['src/modbus/modbus_com.cpp']};
};
