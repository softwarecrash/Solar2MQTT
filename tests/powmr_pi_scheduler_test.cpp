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

    // QFLAG fails forever. QBEQI and all dynamic/static supplement queries
    // must still get polling opportunities.
    for (uint32_t now = 100; now <= 8000; now += 100)
    {
        const auto query = scheduler.next(now, true, false, true);
        if (query == PowMrPiSupplementQuery::None) continue;

        seen.push_back(query);
        scheduler.record(query, now, query != PowMrPiSupplementQuery::QFLAG);
    }

    bool sawQbeqi = false;
    bool sawQpiri = false;
    bool sawQpigs = false;
    bool sawQ1 = false;
    for (const auto query : seen)
    {
        sawQbeqi = sawQbeqi || query == PowMrPiSupplementQuery::QBEQI;
        sawQpiri = sawQpiri || query == PowMrPiSupplementQuery::QPIRI;
        sawQpigs = sawQpigs || query == PowMrPiSupplementQuery::QPIGS;
        sawQ1 = sawQ1 || query == PowMrPiSupplementQuery::Q1;
    }

    assert(scheduler.failures(PowMrPiSupplementQuery::QFLAG) >= 2);
    assert(sawQbeqi);
    assert(sawQpiri);
    assert(sawQpigs);
    assert(sawQ1);

    // Unsupported QBEQI gets the same treatment: its forced refresh remains
    // pending, but retries are backed off and cannot block the other queries.
    PowMrPiSupplementScheduler qbeqiScheduler;
    assert(qbeqiScheduler.next(100, false, false, true) == PowMrPiSupplementQuery::QFLAG);
    qbeqiScheduler.record(PowMrPiSupplementQuery::QFLAG, 100, true);

    bool qbeqiSawQpiri = false;
    bool qbeqiSawQpigs = false;
    bool qbeqiSawQ1 = false;
    for (uint32_t now = 200; now <= 5000; now += 100)
    {
        const auto query = qbeqiScheduler.next(now, false, true, true);
        if (query == PowMrPiSupplementQuery::None) continue;

        if (query == PowMrPiSupplementQuery::QBEQI)
        {
            qbeqiScheduler.record(query, now, false);
            continue;
        }

        qbeqiSawQpiri = qbeqiSawQpiri || query == PowMrPiSupplementQuery::QPIRI;
        qbeqiSawQpigs = qbeqiSawQpigs || query == PowMrPiSupplementQuery::QPIGS;
        qbeqiSawQ1 = qbeqiSawQ1 || query == PowMrPiSupplementQuery::Q1;
        qbeqiScheduler.record(query, now, true);
    }

    assert(qbeqiScheduler.failures(PowMrPiSupplementQuery::QBEQI) >= 2);
    assert(qbeqiSawQpiri);
    assert(qbeqiSawQpigs);
    assert(qbeqiSawQ1);

    return 0;
}
