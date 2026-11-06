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
    uint32_t stallAt = UINT32_MAX, resumeAt = 0;
    bool stalled = false;
    uint32_t now() const { return time; }
    bool cancelled() const { return static_cast<uint32_t>(time-start) >= cancelAt; }
    int read() {
        if (next < bytes.size() && static_cast<uint32_t>(time-start) >= bytes[next].first)
            return bytes[next++].second;
        return -1;
    }
    void pause() {
        if (!stalled && static_cast<uint32_t>(time-start) == stallAt) {
            time = start + resumeAt; stalled = true;
        } else ++time;
        ++pauses;
    }
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
// Replay existing repo payloads through the reader's CR framing. CRC validation
// remains downstream; the existing binary case covers opaque checksum bytes.
static void bufferedRecordedReplies()
{
    const std::pair<const char *, const char *> samples[] = {
        {"MAX QPIGS2 (test_qpigs2.cpp)", "01.9 388.9 00745"},
        {"GK QPIGS2 (pi_fixture.h)",
         "0002 086.4 46.70 00093 10000000 0000 0000 0000 000.0 00.00 00000 00177"},
        {"PI41 QPIGS2 (pi_fixture.h)",
         "230.0 50.0 230.0 50.0 1000 0900 020 0005 081.6 47.20 00100100"},
        {"Q1 (Q1.h recorded 27-field example)",
         "00001 16971 01 00 00 026 033 022 029 02 00 000 0036 0000 0000 00.00 10 0 060 030 100 030 58.40 000 120 0 0000"},
    };
    for (const auto &sample : samples)
        for (bool wrap : {false, true})
        {
            FakeTransport transport;
            if (wrap) transport.start = transport.time = UINT32_MAX - 100;
            const std::string expected = std::string("(") + sample.second;
            for (size_t i = 0; i < expected.size(); ++i)
                transport.bytes.push_back({100 + static_cast<uint32_t>(i) * 5,
                                           static_cast<unsigned char>(expected[i])});
            transport.bytes.push_back({100 + static_cast<uint32_t>(expected.size()) * 5, '\r'});
            transport.stallAt = 100;
            // All remaining bytes arrive during this scheduler stall, well
            // inside the unchanged 2400-baud frame deadline.
            transport.resumeAt = transport.bytes.back().first + 201;
            assert(transport.resumeAt < PiReplyReader::frameTimeoutMs(2400));
            char buffer[PiReplyReader::kMaxReplyBytes];
            const auto result = PiReplyReader::read(transport, buffer, sizeof(buffer),
                PiReplyReader::frameTimeoutMs(2400), PiReplyReader::firstByteTimeoutMs(2400));
            if (result.status != PiReplyReader::Status::Complete)
                std::fprintf(stderr, "Buffered replay rejected: %s, wrap=%d, len=%u, elapsed=%u\n",
                    sample.first, wrap, static_cast<unsigned>(result.length),
                    static_cast<unsigned>(transport.time - transport.start));
            assert(result.status == PiReplyReader::Status::Complete);
            assert(std::string(buffer, result.length) == expected);
            assert(transport.next == expected.size() + 1);
            assert(static_cast<uint32_t>(transport.time - transport.start) == transport.resumeAt);
        }
}

static void readerDeadlinePolicy()
{
    using Reader = PiReplyReader;
    char buffer[Reader::kMaxReplyBytes];
    // A ready byte wins at an idle boundary; a genuinely idle UART still times
    // out there. The absolute frame deadline always takes priority over data.
    FakeTransport firstBoundary; firstBoundary.bytes = {{475, '('}, {475, '\r'}};
    auto result = Reader::read(firstBoundary, buffer, sizeof(buffer), 742, 475);
    assert(result.status == Reader::Status::Complete && result.length == 1 && firstBoundary.time == 475);
    FakeTransport firstLate; firstLate.bytes = {{476, '('}, {476, '\r'}};
    result = Reader::read(firstLate, buffer, sizeof(buffer), 742, 475);
    assert(result.status == Reader::Status::Timeout && result.length == 0 && firstLate.time == 475);
    FakeTransport gapBoundary; gapBoundary.bytes = {{100, '('}, {300, '\r'}};
    result = Reader::read(gapBoundary, buffer, sizeof(buffer), 742, 475);
    assert(result.status == Reader::Status::Complete && result.length == 1 && gapBoundary.time == 300);
    FakeTransport gapLate; gapLate.bytes = {{100, '('}, {301, '\r'}};
    result = Reader::read(gapLate, buffer, sizeof(buffer), 742, 475);
    assert(result.status == Reader::Status::Timeout && result.length == 1 && gapLate.time == 300);

    for (const uint32_t resume : {741U, 742U, 743U})
    {
        FakeTransport capped; capped.bytes = {{100, '('}, {105, 'A'}, {110, '\r'}};
        capped.stallAt = 100; capped.resumeAt = resume;
        result = Reader::read(capped, buffer, sizeof(buffer), 742, 475);
        assert(result.status == (resume < 742 ? Reader::Status::Complete : Reader::Status::Timeout));
        assert(capped.next == (resume < 742 ? 3U : 1U));
    }
    FakeTransport cancelled; cancelled.bytes = {{100, '('}, {105, '\r'}};
    cancelled.stallAt = 100; cancelled.resumeAt = 742; cancelled.cancelAt = 742;
    result = Reader::read(cancelled, buffer, sizeof(buffer), 742, 475);
    assert(result.status == Reader::Status::Cancelled && cancelled.next == 1);

    // The API has no arrival timestamps: after an oversleep, an on-time queued
    // first byte and a physically late queued first byte are indistinguishable.
    // Both may be consumed within the hard cap; this is an observed-idle budget.
    for (const uint32_t arrival : {200U, 550U})
    {
        FakeTransport first; first.bytes = {{arrival, '('}, {arrival + 5, '\r'}};
        first.stallAt = 100; first.resumeAt = 600;
        result = Reader::read(first, buffer, sizeof(buffer), 742, 475);
        assert(result.status == Reader::Status::Complete && result.length == 1 && first.time == 600);
    }
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
    bufferedRecordedReplies();
    readerDeadlinePolicy();
    std::puts("PI serial: slow/binary replies, silence, partial frames, total deadline, overflow, cancellation, wraparound and loopback restoration passed");
}
