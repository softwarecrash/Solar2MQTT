#include "core/FactoryResetManager.h"

#include <esp_system.h>
#include <nvs.h>
#include <nvs_flash.h>

namespace
{
bool s_initialized = false;
bool s_counterClearedThisBoot = false;
uint32_t s_clearTimeoutMs = 15000;
uint32_t s_clearAtMillis = 0;
uint8_t s_pressesRequired = 6;

constexpr const char *RESET_NS = "sys";
constexpr const char *RESET_KEY_CNT = "rstCnt";

esp_err_t initNvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err;
}
} // namespace

void FactoryResetManager::begin(uint32_t clearTimeoutMs, uint8_t pressesRequired)
{
    if (s_initialized)
    {
        return;
    }

    s_clearTimeoutMs = clearTimeoutMs;
    s_pressesRequired = pressesRequired;

    if (initNvs() != ESP_OK)
    {
        Serial.println(F("[FactoryReset] NVS init failed"));
        s_initialized = true;
        return;
    }

    handleBootResetCount();
    s_initialized = true;
}

void FactoryResetManager::loop()
{
    if (!s_initialized)
    {
        return;
    }

    if (!s_counterClearedThisBoot && s_clearAtMillis != 0 && millis() >= s_clearAtMillis)
    {
        clearResetCounter();
        s_counterClearedThisBoot = true;
    }
}

void FactoryResetManager::handleBootResetCount()
{
    const esp_reset_reason_t reason = esp_reset_reason();
    const bool countThisBoot = (reason == ESP_RST_POWERON || reason == ESP_RST_EXT);

    nvs_handle_t handle = {};
    if (nvs_open(RESET_NS, NVS_READWRITE, &handle) != ESP_OK)
    {
        Serial.println(F("[FactoryReset] nvs_open failed"));
        return;
    }

    uint8_t count = 0;
    if (nvs_get_u8(handle, RESET_KEY_CNT, &count) != ESP_OK)
    {
        count = 0;
    }

    if (countThisBoot)
    {
        count++;
        nvs_set_u8(handle, RESET_KEY_CNT, count);
        nvs_commit(handle);
    }

    nvs_close(handle);

    s_counterClearedThisBoot = false;
    s_clearAtMillis = millis() + s_clearTimeoutMs;

    if (countThisBoot && count >= s_pressesRequired)
    {
        clearResetCounter();
        performFactoryReset();
    }
}

void FactoryResetManager::clearResetCounter()
{
    nvs_handle_t handle = {};
    if (nvs_open(RESET_NS, NVS_READWRITE, &handle) != ESP_OK)
    {
        return;
    }

    nvs_set_u8(handle, RESET_KEY_CNT, 0);
    nvs_commit(handle);
    nvs_close(handle);
}

void FactoryResetManager::performFactoryReset()
{
    Serial.println(F("[FactoryReset] Erasing NVS"));
    nvs_flash_erase();
    initNvs();
    delay(200);
    ESP.restart();
}
