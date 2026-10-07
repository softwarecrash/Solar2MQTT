# Host regression checks

From the repository root, with a C++14 compiler:

```sh
c++ -std=c++14 -Wall -Wextra -Werror -Isrc tests/host/test_pi_serial.cpp -o /tmp/test_pi_serial
/tmp/test_pi_serial
```

The test executes the firmware reply reader and loopback settings guard against simulated transports. It covers a reply lasting longer than 500 ms, a 700 ms first-byte delay, baud-dependent budgets, silent devices, incomplete frames, the overall frame deadline, noise overflow, binary CRC bytes, cancellation, clock wraparound, and restoring the actual UART baud/timeout after both loopback outcomes.

PI replies are bounded to 256 bytes. The existing first-byte allowance is retained: 120 bytes of transmission time plus 350 ms (850 ms at 2400 baud, 475 ms at 9600 baud). Stalls between bytes are limited to 200 ms. The overall frame deadline adds the transmission time of the 256-byte buffer to that allowance (1917 ms at 2400 baud, 742 ms at 9600 baud). A silent device stops at the first-byte allowance, not the overall deadline. The receive timers begin after command transmission and the existing 20 ms settling delay. Waiting yields to the scheduler. Only CR-terminated replies reach the existing CRC/NAK parser.

Hardware tests are still required for UART scheduling, unusually long replies, long inter-byte gaps, and inverter-specific turnaround. Modbus has its own response deadline; these PI changes do not increase the Modbus detection timeout.
