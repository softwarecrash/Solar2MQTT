#include "pi_fixture.h"
#include "pi_snapshot_cases.h"
#include "pi_startup_cases.h"
#include "pi_model_cases.h"

int main()
{
    completeSnapshots();
    ownershipAndRecovery();
    changingMeasurements();
    errorsAndCommands();
    concurrentSnapshots();
    firstCycleAndRevoFallback();
    lateStartupRefinement();
    startupRollbackRegressions();
    modbusTimingDelegation();
    legacyModelFailures();
    legacyModelFallback();
    std::puts("Snapshot/publication regressions passed.");
}
