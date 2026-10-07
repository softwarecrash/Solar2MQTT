#include "PI_Serial/PiReplyReader.h"
#include "PI_Serial/SerialSettingsGuard.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <utility>
#include <string>

struct FakeTransport {
    uint32_t time = 0, start = 0;
    std::vector<std::pair<uint32_t, unsigned char>> bytes;
    size_t next = 0;
    unsigned pauses = 0;
    uint32_t cancelAt = UINT32_MAX;
    uint32_t now() const { return time; }
    bool cancelled() const { return static_cast<uint32_t>(time-start) >= cancelAt; }
    int read() {
        if (next < bytes.size() && static_cast<uint32_t>(time-start) >= bytes[next].first)
            return bytes[next++].second;
        return -1;
    }
    void pause() { ++time; ++pauses; }
};
struct FakeSerial {
    uint32_t activeBaud = 9600, timeout = 475, config = 0;
    int8_t rx = 0, tx = 0;
    uint32_t baudRate() const { return activeBaud; }
    uint32_t getTimeout() const { return timeout; }
    void setTimeout(uint32_t value) { timeout = value; }
    void begin(uint32_t baud, uint32_t cfg, int8_t r, int8_t t) { activeBaud=baud;config=cfg;rx=r;tx=t; }
};
static bool loopback(FakeSerial &serial, bool success) {
    SerialSettingsGuard<FakeSerial> restore(serial, 7, 19, 23, 2400);
    serial.begin(restore.baud(), 7, 19, 23);
    serial.setTimeout(200);
    return success;
}
int main() {
    char buffer[256];
    using Reader=PiReplyReader;
    assert(Reader::firstByteTimeoutMs(2400)==850 && Reader::frameTimeoutMs(2400)==1917);
    assert(Reader::firstByteTimeoutMs(9600)==475 && Reader::frameTimeoutMs(9600)==742);
    assert(Reader::firstByteTimeoutMs(0)==850);
    FakeTransport late;
    for (unsigned i=0;i<110;++i) late.bytes.push_back({700+i*5,'A'});
    late.bytes.push_back({1250,'\r'});
    auto lateResult=Reader::read(late,buffer,sizeof(buffer),1917,850);
    assert(lateResult.status==Reader::Status::Complete && lateResult.length==110 && late.time==1250);
    FakeTransport slow;
    for (unsigned i=0;i<110;++i) slow.bytes.push_back({100+i*5,'A'});
    slow.bytes.push_back({650,'\r'});
    auto result=Reader::read(slow,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Complete && result.length==110 && slow.time==650 && slow.pauses==650);
    FakeTransport silent;
    result=Reader::read(silent,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Timeout && result.length==0 && silent.time==850 && silent.pauses==850);
    FakeTransport partial;partial.bytes={{10,'('},{15,'A'}};
    result=Reader::read(partial,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Timeout && result.length==2 && partial.time==215);
    FakeTransport deadline;
    for(unsigned i=0;i<100;++i) deadline.bytes.push_back({i*100,'x'});
    result=Reader::read(deadline,buffer,sizeof(buffer),700,850);
    assert(result.status==Reader::Status::Timeout && deadline.time==700);
    FakeTransport noise;
    for(unsigned i=0;i<257;++i) noise.bytes.push_back({0,'x'});
    result=Reader::read(noise,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Overflow && result.length==256);
    FakeTransport binary;binary.bytes={{0,'('},{0,0x80},{0,0x00},{0,'\r'}};
    result=Reader::read(binary,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Complete && result.length==3 && static_cast<unsigned char>(buffer[1])==0x80 && buffer[2]==0);
    FakeTransport cancelled;cancelled.cancelAt=30;
    result=Reader::read(cancelled,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Cancelled && cancelled.time==30);
    FakeTransport wrap;wrap.start=wrap.time=UINT32_MAX-100;wrap.bytes={{100,'('},{650,'\r'}};
    // Gap >200 ms must fail even across millis() wraparound.
    result=Reader::read(wrap,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Timeout && static_cast<uint32_t>(wrap.time-wrap.start)==300);
    FakeTransport wrapComplete;wrapComplete.start=wrapComplete.time=UINT32_MAX-100;wrapComplete.bytes={{100,'('},{110,'\r'}};
    result=Reader::read(wrapComplete,buffer,sizeof(buffer),1917,850);
    assert(result.status==Reader::Status::Complete && result.length==1);
    FakeSerial serial;
    assert(loopback(serial,true));assert(serial.activeBaud==9600 && serial.timeout==475 && serial.config==7 && serial.rx==19 && serial.tx==23);
    assert(!loopback(serial,false));assert(serial.activeBaud==9600 && serial.timeout==475);
    serial.activeBaud=0;
    loopback(serial,false);assert(serial.activeBaud==2400 && serial.timeout==475);
    std::puts("PI serial: slow/binary replies, silence, partial frames, total deadline, overflow, cancellation, wraparound and loopback restoration passed");
}
