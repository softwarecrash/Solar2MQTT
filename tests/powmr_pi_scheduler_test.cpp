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

    return 0;
}
