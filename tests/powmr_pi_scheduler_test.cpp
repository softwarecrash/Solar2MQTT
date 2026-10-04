#include "../src/PI_Serial/PowMrPiSupplementScheduler.h"

#include <cassert>
#include <vector>

int main()
{
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
