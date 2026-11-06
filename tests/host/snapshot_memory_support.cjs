'use strict';
const {extract}=require('./support/extract.cjs');
const fs=require('fs'),path=require('path');
module.exports=h=>{
 require('./support/mocks.cjs')(h);
 fs.appendFileSync(path.join(h.build,'Arduino.h'),'\n#include <cmath>\nusing std::isfinite;\n');
 h.write('core/LogSerial.h',`#pragma once
struct FakeLogSerial {
 void println(const char *) {}
 template<typename... Args> void printf(const char *, Args...) {}
};
extern FakeLogSerial LogSerial;
`);
 h.write('soc/soc_caps.h','#pragma once\n#define SOC_TEMP_SENSOR_SUPPORTED 1\nfloat temperatureRead();\n');
 h.write('MycilaDS18.h',`#pragma once
#include <cstdint>
#include <cstddef>
// Clang advertises an old GCC compatibility version but provides <optional>.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 7
namespace std {
template <typename T> class optional {
 bool present;
 T value;
public:
 optional() : present(false), value() {}
 optional(T v) : present(true), value(v) {}
 bool has_value() const { return present; }
 T operator*() const { return value; }
};
}
#else
#include <optional>
#endif
extern bool sensorFresh;
extern bool sensorHasTemperature;
extern float sensorTemperature;
class OneWire32 {
public:
 explicit OneWire32(uint8_t) {}
 size_t search(uint64_t *addresses, size_t capacity) { if (!capacity) return 0; addresses[0] = 1; return 1; }
};
namespace Mycila {
class DS18 {
public:
 void begin(OneWire32 *, uint64_t) {}
 void end() {}
 void setExpirationDelay(uint32_t) {}
 bool read() { return sensorFresh; }
 std::optional<float> getTemperature() { return sensorHasTemperature ? std::optional<float>(sensorTemperature) : std::optional<float>(); }
};
}
`);
 const src=h.read('src/core/SolarState.cpp');
 const a=src.indexOf('class SnapshotLock'),b=src.indexOf('\n};',a);
 if(a<0||b<0)throw Error('Missing snapshot lock');
 const get=name=>{for(const r of ['void','bool']){try{return extract(src,`${r} SolarState::${name}`);}catch(e){if(!e.message.startsWith('Missing production function'))throw e;}}throw Error(`Missing SolarState::${name}`);};
 const serializer=src.indexOf('template <typename Writer>\nclass SnapshotJsonSerializer'),serializerEnd=src.indexOf('JsonObject ensureObject',serializer);
 if(serializer<0||serializerEnd<0)throw Error('Missing snapshot serializer');
 h.write('production-snapshot.inc','#include "core/JsonOutput.h"\n'+src.slice(a,b+3)+'\n'+src.slice(serializer,serializerEnd)+'\n'+extract(src,'JsonObject ensureObject')+'\n'+extract(src,'void copyObjectSection')+'\n'+extract(src,'JsonDocument &SolarState::doc')+'\n'+['ensureObjects','refreshBindings','commitInverterState','commitLocalState','snapshotTo','serializeSnapshot','recoverWorkingState'].map(get).join('\n'));
};
