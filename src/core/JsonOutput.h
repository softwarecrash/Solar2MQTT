#pragma once

#include <Arduino.h>

// Keep the caller's reservation: ArduinoJson's String writer clears it.
struct JsonStringWriter
{
    String &output;
    size_t write(uint8_t value)
    {
        return output.concat(static_cast<char>(value)) ? 1 : 0;
    }
    size_t write(const uint8_t *data, size_t length)
    {
        return output.concat(data, length) ? length : 0;
    }
};

struct JsonSizeWriter
{
    size_t size = 0;
    size_t write(uint8_t) { ++size; return 1; }
    size_t write(const uint8_t *, size_t length) { size += length; return length; }
};
