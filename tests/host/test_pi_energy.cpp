#define TEST_PI_ENERGY 1
#include "pi_fixture.h"
#include "pi_energy_cases.h"
#include "pi_snapshot_memory_cases.h"

int main()
{
    freshnessAndEnergy();
    pollingEnergyAndFormats();
    snapshotFailureFreshness();
    std::puts("PI timing/startup regressions passed.");
}
