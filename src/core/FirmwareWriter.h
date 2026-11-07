#pragma once
#include <Update.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Serialize ownership of the single global Update object, without holding the
// mutex during network operations. A committed image stays reserved until reboot.
class FirmwareWriter
{
public:
    // Called during setup before the web server starts. Failed allocation leaves updates disabled.
    void begin() { if (!mutex) mutex = xSemaphoreCreateMutex(); }
    bool acquire(const void *token)
    {
        if (!mutex || !token) return false;
        xSemaphoreTake(mutex, portMAX_DELAY);
        const bool ok = !owner && !committed;
        if (ok) owner = token;
        xSemaphoreGive(mutex);
        return ok;
    }
    void cancel(const void *token)
    {
        if (!mutex) return;
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (owner == token && !committed)
        {
            if (Update.isRunning()) Update.abort();
            owner = nullptr;
        }
        xSemaphoreGive(mutex);
    }
    void commit(const void *token)
    {
        if (!mutex) return;
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (owner == token) committed = true;
        xSemaphoreGive(mutex);
    }
private:
    SemaphoreHandle_t mutex = nullptr;
    const void *owner = nullptr;
    bool committed = false;
};
extern FirmwareWriter firmwareWriter;

class FirmwareWriterLease
{
public:
    explicit FirmwareWriterLease(const void *token) : token(token), acquired(firmwareWriter.acquire(token)) {}
    FirmwareWriterLease(const FirmwareWriterLease &) = delete;
    FirmwareWriterLease &operator=(const FirmwareWriterLease &) = delete;
    ~FirmwareWriterLease() { if (acquired) firmwareWriter.cancel(token); }
    void commit() { firmwareWriter.commit(token); }
    const void *token;
    bool acquired;
};
