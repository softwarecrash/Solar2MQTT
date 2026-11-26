#include <Arduino.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include "led_mode.h"
#define private public
#include "core/StatusLedService.h"
#undef private
#include "core/StatusLedService.cpp"
int pwmAttach=0,pwmDetach=0,pwmWrites=0,lastPwm=-1;
int rmtAttach=0,rmtDetach=0,rgbWrites=0,lastRgb=-1;
bool rmtOk=true;
uint32_t fakeNow=1;
int ticksLeft=250;

int main() {
    StatusLedService led;
    led.begin(-1,128);
    const auto task=led._taskHandle;
    led.applyConfiguration();
    assert(!led._attached && pwmAttach==0 && rmtAttach==0);
    assert(led.determinePulseCount(false,true,false,false)==4);
    assert(led.determinePulseCount(true,true,false,false)==3);
    assert(led.determinePulseCount(true,true,true,false)==2);
    assert(led.determinePulseCount(true,true,true,true)==1);
#if defined(STATUS_LED_ADDRESSABLE) && STATUS_LED_ADDRESSABLE
    led.configure(27,128);led.applyConfiguration();
    assert(pwmAttach==0 && rmtAttach==1 && lastRgb==0);
    led.writeBrightness(128);
    const int writes=rgbWrites;
    for(int i=0;i<250;++i) led.writeBrightness(128);
    assert(rgbWrites==writes); // Hold/off windows never re-send identical frames.
    led.configure(27,0);led.applyConfiguration();led.writeBrightness(0);
    assert(lastRgb==0 && led._activeBrightness==0);
    led.configure(-1,128);led.applyConfiguration();
    assert(!led._attached && rmtDetach==1);
    rmtOk=false;led.configure(27,128);led.applyConfiguration();
    assert(!led._attached);
    const int before=rgbWrites;led.writeBrightness(128);assert(rgbWrites==before);
    led.configure(-1,128);led.applyConfiguration();rmtOk=true;
    led.configure(27,128);led.applyConfiguration();assert(led._attached && lastRgb==0);
    led.configure(2,128);led.applyConfiguration();
    assert(rmtDetach==2 && rmtAttach==4 && pwmAttach==0);
    led.writeBrightness(64);assert(lastRgb==64);
#else
    led.configure(2,128);led.applyConfiguration();led.writeBrightness(64);
    assert(led._attached && pwmAttach==1 && lastPwm==64);
    led.configure(-1,128);led.applyConfiguration();
    assert(!led._attached && pwmDetach==1 && lastPwm==0);
    assert(rmtAttach==0 && rgbWrites==0);
#endif
    led.begin(2,64);assert(led._taskHandle==task);
#if defined(STATUS_LED_ADDRESSABLE) && STATUS_LED_ADDRESSABLE
    for(int pulses=1;pulses<=4;++pulses) {
        StatusLedService cycle;
        cycle.begin(27,128);
        cycle.loop(pulses!=4,true,pulses!=3,pulses!=2);
        fakeNow=1;ticksLeft=250;
        const int start=rgbWrites;
        try {cycle.taskLoop();} catch(const EndCycle &) {}
        const int writes=rgbWrites-start;
        assert(writes<=250);
        printf("RGB schedule: %d pulses, %d frames per 5 seconds (brightness 128).\n",pulses,writes);
        cycle.configure(-1,0);cycle.applyConfiguration();
    }
#endif
    puts("Status LED: pulse mapping, disable, PWM/RGB switch, init failure, resource release and frame deduplication passed.");
}
