#pragma once
#include <stdint.h>

// Automatic, optional mapping: 4563 = 0.1 V, 4564 = W, byte-swapped.
// Verified on Daxtromn VMII-NXPW5KW; a generic HVM model name does not
// establish support. Publish only after a plausible active PV2 voltage.
class PowMrPv2
{
public:
    enum class ReadResult { Ok, Unsupported, Failed };
    static ReadResult classifyFailure(uint8_t result)
    {
        // PowMr variants may set bit 7 in the exception code itself.
        const uint8_t exception = result & 0x7F;
        return exception >= 1 && exception <= 3 ? ReadResult::Unsupported : ReadResult::Failed;
    }
    static constexpr uint16_t kResponseTimeoutMs = 250;
    static constexpr uint32_t kFailureBackoffMs = 5UL * 60UL * 1000UL;
    static constexpr uint32_t kInactiveProbeMs = 60UL * 1000UL;

    template <typename ReadBlock, typename Publish, typename Clear>
    void poll(uint32_t now, ReadBlock read, Publish publish, Clear clear)
    {
        if (_unsupported || (_attempted && static_cast<uint32_t>(now - _lastAttempt) < _retryDelay)) return;
        _attempted = true;
        _lastAttempt = now;
        uint16_t words[2] = {};
        // One request for both values. No retries or connection-counter
        // changes; permanent exceptions suppress further probes this session.
        const ReadResult result = read(words);
        if (result != ReadResult::Ok)
        {
            _unsupported = result == ReadResult::Unsupported;
            _retryDelay = kFailureBackoffMs;
            clear();
            return;
        }
        const uint16_t voltage10 = swap(words[0]);
        if (voltage10 > 10000)
        {
            _retryDelay = kFailureBackoffMs;
            clear();
            return;
        }
        // All-zero/low registers do not prove a second tracker exists. Probe
        // slowly until daylight; once confirmed, zero at night is valid.
        if (!_confirmed && voltage10 < 300)
        {
            _retryDelay = kInactiveProbeMs;
            clear();
            return;
        }
        _confirmed = true;
        _retryDelay = 0;
        publish(voltage10 / 10.0f, swap(words[1]));
    }

private:
    static uint16_t swap(uint16_t value)
    {
        return static_cast<uint16_t>((value >> 8) | (value << 8));
    }
    bool _unsupported = false;
    bool _confirmed = false;
    bool _attempted = false;
    uint32_t _lastAttempt = 0;
    uint32_t _retryDelay = 0;
};
