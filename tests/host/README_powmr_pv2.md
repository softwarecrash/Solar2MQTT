# Host regression checks

From the repository root, with a C++14 compiler and Node.js:

```sh
c++ -std=c++14 -Wall -Wextra -Werror -Isrc tests/host/test_powmr_pv2.cpp -o /tmp/test_powmr_pv2
/tmp/test_powmr_pv2
node tests/host/test_pv_total.cjs
```

The C++ test executes the same optional PV2 policy used by the firmware with a simulated register reader. It covers success, byte swapping/scaling, one attempt on permanent exceptions, timed recovery after transient failures, stale-value clearing, startup at night, invalid voltage, and clock wraparound. The JavaScript test uses the functions extracted from the actual UI source, including legacy profiles and zero totals.

The PV2 register mapping is empirical. Plausibility checks cannot prove that other undocumented HVM variants use the same addresses. Hardware testing remains necessary, especially for response time and model compatibility. The 250 ms budget begins after request transmission; it is not a complete transaction-time guarantee.
