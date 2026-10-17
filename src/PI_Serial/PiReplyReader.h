#pragma once
#include <stddef.h>
#include <stdint.h>

// Transport supplies now(), read(), pause() and cancelled(). Keep this reader
// independent of Arduino so timing and partial/noisy replies can be tested.
struct PiReplyReader
{
    static constexpr size_t kMaxReplyBytes = 256;
    static constexpr uint32_t kTurnaroundAllowanceMs = 350;
    static constexpr uint32_t kExpectedReplyBytes = 120;
    static uint32_t firstByteTimeoutMs(uint32_t baud)
    {
        if (baud == 0) baud = 2400;
        return (kExpectedReplyBytes * 10UL * 1000UL + baud - 1) / baud + kTurnaroundAllowanceMs;
    }
    static uint32_t frameTimeoutMs(uint32_t baud)
    {
        if (baud == 0) baud = 2400;
        return firstByteTimeoutMs(baud) + (kMaxReplyBytes * 10UL * 1000UL + baud - 1) / baud;
    }
    static constexpr uint32_t kInterByteTimeoutMs = 200;
    enum class Status { Complete, Timeout, Overflow, Cancelled };
    struct Result { Status status; size_t length; };

    template <typename Transport>
    static Result read(Transport &transport, char *buffer, size_t capacity, uint32_t frameTimeoutMs, uint32_t firstByteTimeoutMs)
    {
        const uint32_t start = transport.now();
        uint32_t lastByte = start;
        size_t length = 0;
        for (;;)
        {
            if (transport.cancelled()) return {Status::Cancelled, length};
            const uint32_t now = transport.now();
            const uint32_t gapLimit = length == 0 ? firstByteTimeoutMs : kInterByteTimeoutMs;
            if (static_cast<uint32_t>(now - start) >= frameTimeoutMs ||
                static_cast<uint32_t>(now - lastByte) >= gapLimit)
                return {Status::Timeout, length};
            const int value = transport.read();
            if (value >= 0)
            {
                if (value == '\r') return {Status::Complete, length};
                if (length >= capacity) return {Status::Overflow, length};
                buffer[length++] = static_cast<char>(value);
                lastByte = now;
            }
            else
            {
                // Yield while waiting rather than spinning inside Stream::timedRead().
                transport.pause();
            }
        }
    }
};
