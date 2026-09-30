#include "../src/modbus/powmr_hunt.h"
#include <cassert>
#include <set>
#include <utility>
int main() {
    PowmrHunt h;
    assert(!h.start(10, 9)); assert(!h.start(0, 65536));
    assert(h.start(65534, 65535));
    std::set<std::pair<int,int>> visited;
    while(h.running) h.step([&](uint8_t f,uint16_t r,uint16_t &v) {
        assert(visited.insert({f,r}).second); v=30; return uint8_t(0);
    });
    assert(h.cursor==4 && h.count==4 && h.valid);
    assert(h.compare());
    while(h.running) h.step([](uint8_t,uint16_t,uint16_t &v){v=35;return uint8_t(0);});
    assert(h.entries[0].baseline==30 && h.entries[0].current==35);
    assert(h.compare());
    while(h.running) h.step([](uint8_t,uint16_t,uint16_t &v){v=30;return uint8_t(0);});
    assert(h.entries[0].baseline==30 && h.entries[0].current==30);
    assert(h.compare());
    int attempts=0;
    while(h.running) h.step([&](uint8_t,uint16_t,uint16_t &){++attempts;return uint8_t(0xE2);});
    assert(attempts==8 && h.errors==4 && h.entries[0].baseline==30 && h.entries[0].error==0xE2);
    assert(h.start(0,65535)); visited.clear();
    while(h.running) h.step([&](uint8_t f,uint16_t r,uint16_t &) {
        if(visited.empty()) assert(r==4000 && f==3);
        assert(visited.insert({f,r}).second); return uint8_t(2);
    });
    assert(visited.size()==131072 && h.cursor==131072 && h.rejected==131072);
    assert(!h.valid && !h.compare());
    assert(h.start(0,65535));
    while(h.running) h.step([](uint8_t,uint16_t,uint16_t &v){v=70;return uint8_t(0);});
    assert(h.count==4096 && h.cursor==4096 && h.valid);
    assert(h.compare()); h.stop();
    assert(h.entries[0].error==0xFF && h.entries[0].baseline==70);
    assert(h.start(4000,4001));
    h.step([](uint8_t,uint16_t,uint16_t &v){v=35;return uint8_t(0);});
    h.stop(); assert(h.valid && h.baselineVisited==1 && h.baselineTotal==4);
}
