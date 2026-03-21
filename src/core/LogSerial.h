#pragma once

#include <Arduino.h>
#include <MycilaWebSerial.h>

class AsyncWebServer;

class LogSerialClass : public Stream
{
public:
    WebSerial webSerial;

    void begin(unsigned long baud = 115200);
    void begin(AsyncWebServer *server, unsigned long baud = 115200, size_t bufferSize = 256);
    void onMessage(std::function<void(const std::string &)> cb);
    void setBuffer(size_t size);

    int available() override;
    int read() override;
    int peek() override;
    void flush() override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t *buffer, size_t size) override;

    using Print::write;
    operator bool();

private:
    bool _started = false;
};

extern LogSerialClass LogSerial;
