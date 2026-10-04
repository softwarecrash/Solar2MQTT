#pragma once

#include <stdint.h>

enum class PowMrPiSupplementQuery : uint8_t
{
    None = 0,
    QFLAG,
    QBEQI,
    QPIRI,
    QPIGS,
    Q1,
};

class PowMrPiSupplementScheduler
{
public:
    PowMrPiSupplementQuery next(uint32_t now,
                                bool forceQflag,
                                bool forceQbeqi = false,
                                bool includeQbeqi = false) const
    {
        if (due(_qflag, now, 60000UL, forceQflag)) return PowMrPiSupplementQuery::QFLAG;
        if (includeQbeqi && due(_qbeqi, now, 60000UL, forceQbeqi)) return PowMrPiSupplementQuery::QBEQI;
        if (due(_qpiri, now, 300000UL, false)) return PowMrPiSupplementQuery::QPIRI;
        if (due(_qpigs, now, 5000UL, false)) return PowMrPiSupplementQuery::QPIGS;
        if (due(_q1, now, 2000UL, false)) return PowMrPiSupplementQuery::Q1;
        return PowMrPiSupplementQuery::None;
    }

    void record(PowMrPiSupplementQuery query, uint32_t now, bool success)
    {
        QueryState *state = stateFor(query);
        if (state == nullptr) return;

        state->attempted = true;
        state->lastAttemptAt = now;
        if (success)
        {
            state->lastSuccessAt = now;
            state->failures = 0;
        }
        else if (state->failures < 255)
        {
            ++state->failures;
        }
    }

    uint8_t failures(PowMrPiSupplementQuery query) const
    {
        const QueryState *state = stateFor(query);
        return state == nullptr ? 0 : state->failures;
    }

private:
    struct QueryState
    {
        uint32_t lastAttemptAt = 0;
        uint32_t lastSuccessAt = 0;
        uint8_t failures = 0;
        bool attempted = false;
    };

    QueryState _qflag;
    QueryState _qbeqi;
    QueryState _qpiri;
    QueryState _qpigs;
    QueryState _q1;

    static uint32_t retryBackoffMs(uint8_t failures)
    {
        if (failures <= 1) return 2000UL;
        if (failures == 2) return 5000UL;
        if (failures == 3) return 10000UL;
        return 30000UL;
    }

    static bool due(const QueryState &state,
                    uint32_t now,
                    uint32_t normalIntervalMs,
                    bool forced)
    {
        if (!state.attempted) return true;

        if (state.failures != 0)
        {
            return static_cast<uint32_t>(now - state.lastAttemptAt) >= retryBackoffMs(state.failures);
        }

        if (forced) return true;
        return static_cast<uint32_t>(now - state.lastSuccessAt) >= normalIntervalMs;
    }

    QueryState *stateFor(PowMrPiSupplementQuery query)
    {
        switch (query)
        {
        case PowMrPiSupplementQuery::QFLAG: return &_qflag;
        case PowMrPiSupplementQuery::QBEQI: return &_qbeqi;
        case PowMrPiSupplementQuery::QPIRI: return &_qpiri;
        case PowMrPiSupplementQuery::QPIGS: return &_qpigs;
        case PowMrPiSupplementQuery::Q1: return &_q1;
        default: return nullptr;
        }
    }

    const QueryState *stateFor(PowMrPiSupplementQuery query) const
    {
        switch (query)
        {
        case PowMrPiSupplementQuery::QFLAG: return &_qflag;
        case PowMrPiSupplementQuery::QBEQI: return &_qbeqi;
        case PowMrPiSupplementQuery::QPIRI: return &_qpiri;
        case PowMrPiSupplementQuery::QPIGS: return &_qpigs;
        case PowMrPiSupplementQuery::Q1: return &_q1;
        default: return nullptr;
        }
    }
};
