'use strict';
const {harness} = require('./support/native.cjs');
function runTests() {
  const h = harness('status-led');
  try {
    h.write('Arduino.h', `#pragma once
#include <cstdint>
using TaskHandle_t=void*;
using BaseType_t=int;
constexpr int pdPASS=1, OUTPUT=1, LOW=0;
inline int xTaskCreate(void (*)(void*),const char*,int,void*,int,TaskHandle_t *out){*out=(void*)1;return pdPASS;}
extern uint32_t fakeNow;
inline uint32_t millis(){return fakeNow;}
inline uint32_t pdMS_TO_TICKS(uint32_t n){return n;}
extern int ticksLeft;
struct EndCycle {};
inline void vTaskDelay(uint32_t ms){fakeNow+=ms;if(--ticksLeft==0)throw EndCycle{};}
inline void pinMode(uint8_t,int){}
inline void digitalWrite(uint8_t,int){}
extern int pwmAttach,pwmDetach,pwmWrites,lastPwm;
inline bool ledcAttach(uint8_t,uint32_t,uint8_t){++pwmAttach;return true;}
inline void ledcDetach(uint8_t){++pwmDetach;}
inline void ledcWrite(uint8_t,uint8_t n){++pwmWrites;lastPwm=n;}
`);
    h.write('core/LogSerial.h', `#pragma once
struct Logger {void println(const char*){} template<class... T> void printf(const char*,T...){} };
Logger LogSerial;
`);
    h.write('esp32-hal-rmt.h', `#pragma once
constexpr int RMT_TX_MODE=1,RMT_MEM_NUM_BLOCKS_1=1;
extern int rmtAttach,rmtDetach; extern bool rmtOk;
inline bool rmtInit(int,int,int,uint32_t){++rmtAttach;return rmtOk;}
inline void rmtDeinit(int){++rmtDetach;}
`);
    h.write('esp32-hal-rgb-led.h', `#pragma once
#include <cassert>
constexpr int LED_COLOR_ORDER_GRB=0;
extern int rgbWrites,lastRgb;
inline void rgbLedWriteOrdered(uint8_t pin,int order,uint8_t r,uint8_t g,uint8_t b){
assert(pin<34 && order==LED_COLOR_ORDER_GRB && r==0 && b==0);++rgbWrites;lastRgb=g;}
`);
    for (const mode of [null, 0, 1]) {
      h.write('led_mode.h', mode === null ? '' : '#define STATUS_LED_ADDRESSABLE ' + mode + '\n');
      h.run(h.compile('tests/host/test_status_led.cpp'), []);
    }
  } finally { h.cleanup(); }
}
module.exports={runTests};
if(require.main===module) runTests();
