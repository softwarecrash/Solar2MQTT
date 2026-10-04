#include "../src/PI_Serial/PowMrPiSupplementScheduler.h"

#include <cassert>
#include <vector>

static void testSlowPasses(bool includeQbeqi, PowMrPiSupplementQuery failingQuery,
                           uint32_t passInterval, uint32_t start)
{
    PowMrPiSupplementScheduler scheduler;
    unsigned counts[6] = {};
    unsigned lastQ1Pass = 0;
    for (unsigned pass = 1; pass <= 120; ++pass)
    {
        const uint32_t now = start + pass * passInterval;
        const auto query = scheduler.next(now,
            failingQuery == PowMrPiSupplementQuery::QFLAG,
            failingQuery == PowMrPiSupplementQuery::QBEQI, includeQbeqi);
        assert(query != PowMrPiSupplementQuery::None);
        assert(includeQbeqi || query != PowMrPiSupplementQuery::QBEQI);
        ++counts[static_cast<unsigned>(query)];
        scheduler.record(query, now, query != failingQuery);
        if (query == PowMrPiSupplementQuery::Q1) lastQ1Pass = pass;

        // Q1 is due on every slow pass. Even with other due/failed queries,
        // it must get an opportunity within one round of at most five queries.
        assert(pass - lastQ1Pass < 5);
    }
    assert(counts[static_cast<unsigned>(PowMrPiSupplementQuery::QFLAG)] > 1);
    assert(counts[static_cast<unsigned>(PowMrPiSupplementQuery::QPIRI)] > 1);
    assert(counts[static_cast<unsigned>(PowMrPiSupplementQuery::QPIGS)] > 1);
    if (includeQbeqi)
        assert(counts[static_cast<unsigned>(PowMrPiSupplementQuery::QBEQI)] > 1);
}

int main()
{
    for (bool includeQbeqi : {false, true})
        for (auto failingQuery : {PowMrPiSupplementQuery::None,
                                  PowMrPiSupplementQuery::QFLAG,
                                  PowMrPiSupplementQuery::QBEQI,
                                  PowMrPiSupplementQuery::QPIGS})
            for (uint32_t interval : {8500U, 35000U, 85000U})
                for (uint32_t start : {0U, UINT32_MAX - 20000U})
                    testSlowPasses(includeQbeqi, failingQuery, interval, start);

    PowMrPiSupplementScheduler scheduler;
    std::vector<PowMrPiSupplementQuery> seen;

    // QFLAG fails every time. The scheduler must back it off instead of
    // retrying it on every Modbus pass and starving the remaining queries.
    for (uint32_t now = 100; now <= 8000; now += 100)
    {
        const auto query = scheduler.next(now, true);
        if (query == PowMrPiSupplementQuery::None)
        {
            continue;
        }

        seen.push_back(query);
        const bool success = query != PowMrPiSupplementQuery::QFLAG;
        scheduler.record(query, now, success);
    }

    bool sawQpiri = false;
    bool sawQpigs = false;
    bool sawQ1 = false;
    for (const auto query : seen)
    {
        sawQpiri = sawQpiri || query == PowMrPiSupplementQuery::QPIRI;
        sawQpigs = sawQpigs || query == PowMrPiSupplementQuery::QPIGS;
        sawQ1 = sawQ1 || query == PowMrPiSupplementQuery::Q1;
    }

    assert(scheduler.failures(PowMrPiSupplementQuery::QFLAG) >= 2);
    assert(sawQpiri);
    assert(sawQpigs);
    assert(sawQ1);

    // The first retry is delayed by 2 seconds.
    PowMrPiSupplementScheduler retry;
    assert(retry.next(100, true) == PowMrPiSupplementQuery::QFLAG);
    retry.record(PowMrPiSupplementQuery::QFLAG, 100, false);
    assert(retry.next(200, true) == PowMrPiSupplementQuery::QPIRI);
    retry.record(PowMrPiSupplementQuery::QPIRI, 200, true);
    retry.record(PowMrPiSupplementQuery::QPIGS, 300, true);
    retry.record(PowMrPiSupplementQuery::Q1, 400, true);
    assert(retry.next(1999, true) == PowMrPiSupplementQuery::None);
    assert(retry.next(2100, true) == PowMrPiSupplementQuery::QFLAG);

    return 0;
}
