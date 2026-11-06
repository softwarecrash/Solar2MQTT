#include "energy_fixture.h"
#include "battery_storage_cases.h"
int main()
{
    battery_cases::persistence();
    battery_cases::interruptedWritesAndReset();
    battery_cases::migration();
    battery_cases::boundedCheckpoints();
}
