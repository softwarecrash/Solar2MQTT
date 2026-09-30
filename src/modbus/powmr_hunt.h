#pragma once
#include <cstdint>
#include <memory>
#include <new>

// Incremental, read-only discovery. The caller owns the UART and calls step()
// between normal telemetry transactions; no worker task touches the serial bus.
class PowmrHunt {
public:
    struct Entry { uint16_t reg, baseline, current; uint8_t function, error; };
    static constexpr uint32_t capacity = 4096;
    std::unique_ptr<Entry[]> entries;
    uint32_t count = 0, cursor = 0, total = 0, rejected = 0, errors = 0;
    uint32_t first = 0, last = 65535, baselineVisited = 0, baselineTotal = 0;
    uint32_t baselineErrors = 0;
    bool running = false, comparing = false, valid = false;
    const char *outcome = "EMPTY";
    const char *coverage = "EMPTY";

    bool start(uint32_t begin, uint32_t end) {
        if (running || begin > end || end > 65535) return false;
        if (!entries) entries.reset(new (std::nothrow) Entry[capacity]);
        if (!entries) return false;
        first = begin; last = end; count = cursor = rejected = errors = 0;
        total = (last - first + 1) * 2;
        baselineVisited = baselineErrors = 0; baselineTotal = total;
        comparing = valid = false; running = true; retry = 0;
        outcome = coverage = "RUNNING";
        return true;
    }
    bool compare() {
        if (running || !valid || !count) return false;
        comparing = running = true; cursor = rejected = errors = retry = 0;
        total = count; outcome = "RUNNING";
        // 0xFF means not sampled, never a value of zero.
        for (uint32_t i = 0; i < count; ++i) entries[i].error = 0xFF;
        return true;
    }
    void stop(const char *reason = "CANCELLED") {
        running = false; outcome = reason;
        if (!comparing) {
            baselineVisited = cursor; baselineErrors = errors;
            coverage = reason; valid = count > 0;
        }
    }
    template<class Reader> void step(Reader read) {
        if (!running) return;
        uint16_t reg;
        uint8_t function;
        if (comparing) {
            reg = entries[cursor].reg; function = entries[cursor].function;
        } else {
            const uint32_t span = last - first + 1;
            // Start at the known PowMr area, then wrap without missing addresses.
            const uint32_t offset = first <= 4000 && last >= 4000 ? 4000 - first : 0;
            reg = static_cast<uint16_t>(first + (cursor / 2 + offset) % span);
            function = cursor % 2 == 0 ? 3 : 4;
        }
        uint16_t value = 0;
        const uint8_t result = read(function, reg, value);
        const bool exception = (result >= 1 && result <= 4) ||
                               (result >= 0x81 && result <= 0x84);
        // Retry a transport error once on a later step; do not retry exceptions.
        if (result && !exception && retry++ == 0) return;
        retry = 0;
        if (comparing) {
            entries[cursor].error = result;
            if (!result) entries[cursor].current = value;
        } else if (!result) {
            entries[count++] = {reg, value, value, function, 0};
        }
        if (result) { if (exception) ++rejected; else ++errors; }
        ++cursor;
        if (!comparing && count == capacity && cursor < total) stop("CAPACITY_LIMIT");
        else if (cursor == total) stop("DONE");
    }
private:
    uint8_t retry = 0;
};
