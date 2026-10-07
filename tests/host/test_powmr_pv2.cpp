#include "modbus/device/powmr/PowMrPv2.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <initializer_list>
struct Device {
    PowMrPv2 driver;
    PowMrPv2::ReadResult result=PowMrPv2::ReadResult::Ok;
    uint16_t voltage10=3140,power=1075;
    int reads=0,published=0,cleared=0;
    float outputVoltage=0;uint16_t outputPower=0;
    static uint16_t swap(uint16_t n) { return static_cast<uint16_t>((n>>8)|(n<<8)); }
    void poll(uint32_t now) {
        driver.poll(now,[&](uint16_t *words) { ++reads;words[0]=swap(voltage10);words[1]=swap(power);return result; },
            [&](float voltage,uint16_t watts) { ++published;outputVoltage=voltage;outputPower=watts; },[&]() { ++cleared; });
    }
};
int main() {
    for (uint8_t code : {1, 2, 3, 0x81, 0x82, 0x83})
        assert(PowMrPv2::classifyFailure(code)==PowMrPv2::ReadResult::Unsupported);
    for (uint8_t code : {0, 4, 0x84, 0xE0, 0xE1, 0xE2, 0xE3})
        assert(PowMrPv2::classifyFailure(code)==PowMrPv2::ReadResult::Failed);
    Device good;good.poll(0);good.poll(1000);
    assert(good.reads==2 && good.published==2 && std::abs(good.outputVoltage-314.0f)<0.01f && good.outputPower==1075);
    good.voltage10=0;good.power=0;good.poll(2000);assert(good.published==3 && good.outputVoltage==0);
    Device absent;absent.result=PowMrPv2::ReadResult::Unsupported;absent.poll(0);
    for(uint32_t now=1000;now<1000000;now+=1000) absent.poll(now);
    assert(absent.reads==1 && absent.published==0 && absent.cleared==1);
    Device transient;transient.result=PowMrPv2::ReadResult::Failed;transient.poll(0);transient.poll(299999);
    assert(transient.reads==1 && transient.cleared==1);
    transient.result=PowMrPv2::ReadResult::Ok;transient.poll(300000);assert(transient.reads==2 && transient.published==1);
    transient.result=PowMrPv2::ReadResult::Failed;transient.poll(301000);assert(transient.cleared==2);transient.poll(302000);assert(transient.reads==3);
    Device night;night.voltage10=0;night.power=0;night.poll(0);night.poll(59999);assert(night.reads==1 && night.published==0);
    night.voltage10=3200;night.power=900;night.poll(60000);assert(night.published==1 && night.outputPower==900);
    Device invalid;invalid.voltage10=65535;invalid.poll(0);invalid.poll(1000);assert(invalid.published==0 && invalid.reads==1);
    Device wrap;wrap.result=PowMrPv2::ReadResult::Failed;wrap.poll(UINT32_MAX-1000);wrap.poll(1000);assert(wrap.reads==1);
    wrap.result=PowMrPv2::ReadResult::Ok;wrap.poll(299000);assert(wrap.published==1);
    std::puts("PowMr PV2: single optional probe, byte order/scaling, permanent exceptions, transient backoff/recovery, night/day, invalid data and wraparound passed");
}
