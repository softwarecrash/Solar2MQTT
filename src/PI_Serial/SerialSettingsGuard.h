#pragma once
#include <stdint.h>

// Loopback must restore the actual UART (including a Modbus-selected baud),
// not just the PI driver's cached fallback. Restore on success and failure.
template <typename Serial>
class SerialSettingsGuard
{
public:
    SerialSettingsGuard(Serial &serial, uint32_t config, int8_t rx, int8_t tx, uint32_t fallbackBaud)
        : _serial(serial), _config(config), _rx(rx), _tx(tx), _timeout(serial.getTimeout()), _baud(serial.baudRate())
    {
        if (_baud == 0) _baud = fallbackBaud == 0 ? 2400 : fallbackBaud;
    }
    ~SerialSettingsGuard()
    {
        _serial.begin(_baud, _config, _rx, _tx);
        _serial.setTimeout(_timeout);
    }
    uint32_t baud() const { return _baud; }
    SerialSettingsGuard(const SerialSettingsGuard &) = delete;
    SerialSettingsGuard &operator=(const SerialSettingsGuard &) = delete;
private:
    Serial &_serial;
    uint32_t _config;
    int8_t _rx;
    int8_t _tx;
    uint32_t _timeout;
    uint32_t _baud;
};
