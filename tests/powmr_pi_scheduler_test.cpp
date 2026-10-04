#include "../src/PI_Serial/PowMrPiSupplementScheduler.h"

#include <cassert>
#include <vector>

int main()
{
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
