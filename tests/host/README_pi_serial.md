# Host regression checks

From the repository root, with a C++14 compiler:

```sh
c++ -std=c++14 -Wall -Wextra -Werror -Isrc tests/host/test_pi_serial.cpp -o /tmp/test_pi_serial
/tmp/test_pi_serial
```

The test executes the firmware reply reader and loopback settings guard against simulated transports. It covers a reply lasting longer than 500 ms, silent devices, incomplete frames, the overall frame deadline, noise overflow, binary CRC bytes, cancellation, clock wraparound, and restoring the actual UART baud/timeout after both loopback outcomes.

PI replies are bounded to 256 bytes. Waiting for the first byte is limited to 500 ms, stalls between bytes to 200 ms, and the overall frame deadline to the transmission time of 256 bytes at the configured baud plus 500 ms (1567 ms at 2400 baud). The receive timers begin after command transmission and the existing 20 ms settling delay. Waiting yields to the scheduler. Only CR-terminated replies reach the existing CRC/NAK parser.

Hardware tests are still required for UART scheduling, unusually long replies, long inter-byte gaps, and inverter-specific turnaround. Modbus has its own response deadline; these PI changes do not increase the Modbus detection timeout.
