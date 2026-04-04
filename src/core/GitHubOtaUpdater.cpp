#include "core/GitHubOtaUpdater.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace
{
constexpr uint32_t kOtaTaskStack = 8192;
constexpr uint32_t kHttpTimeoutMs = 15000;
constexpr const char *kUserAgent = "Solar2MQTT-OTA";
}

GitHubOtaUpdater::GitHubOtaUpdater(const char *owner,
                                   const char *repo,
                                   const char *currentVersion,
                                   const char *buildVariant)
    : _owner(owner ? owner : ""),
      _repo(repo ? repo : ""),
      _currentVersion(currentVersion ? currentVersion : ""),
      _buildVariant(buildVariant ? buildVariant : "")
{
}

void GitHubOtaUpdater::begin(std::function<bool()> networkConnected)
{
    _networkConnected = std::move(networkConnected);
    if (_lockHandle == nullptr)
    {
        _lockHandle = xSemaphoreCreateMutex();
    }
}

bool GitHubOtaUpdater::isBusy() const
{
    lock();
    const bool busy = (_state == State::Checking || _state == State::Downloading);
    unlock();
    return busy;
}

bool GitHubOtaUpdater::requestCheck()
{
    lock();
    const bool busy = (_state == State::Checking || _state == State::Downloading);
    if (busy || _task != nullptr)
    {
        unlock();
        return false;
    }
    _bytesDone = 0;
    _bytesTotal = 0;
    _latestVersion = "";
    _assetUrl = "";
    _assetName = "";
    unlock();

    setState(State::Checking, "");

    TaskHandle_t handle = nullptr;
    if (xTaskCreate(checkTask, "solar_ota_check", kOtaTaskStack, this, 1, &handle) != pdPASS)
    {
        setState(State::Error, "Failed to start update check task");
        return false;
    }

    lock();
    _task = handle;
    unlock();
    return true;
}

bool GitHubOtaUpdater::startUpdate()
{
    lock();
    const bool validState = (_state == State::UpdateAvailable);
    const bool busy = (_state == State::Checking || _state == State::Downloading);
    if (!validState || busy || _task != nullptr)
    {
        unlock();
        return false;
    }
    _bytesDone = 0;
    _bytesTotal = 0;
    unlock();

    setState(State::Downloading, "");

    TaskHandle_t handle = nullptr;
    if (xTaskCreate(downloadTask, "solar_ota_dl", kOtaTaskStack, this, 1, &handle) != pdPASS)
    {
        setState(State::Error, "Failed to start update task");
        return false;
    }

    lock();
    _task = handle;
    unlock();
    return true;
}

String GitHubOtaUpdater::statusJson() const
{
    String current;
    String latest;
    String asset;
    String error;
    uint32_t total = 0;
    uint32_t done = 0;
    State state = State::Idle;

    lock();
    current = _currentVersion;
    latest = _latestVersion;
    asset = _assetName;
    error = _lastError;
    total = _bytesTotal;
    done = _bytesDone;
    state = _state;
    unlock();

    const uint8_t progress = (total > 0) ? static_cast<uint8_t>((done * 100ULL) / total) : 0;

    JsonDocument doc;
    doc["state"] = (state == State::Idle) ? "idle"
                  : (state == State::Checking) ? "checking"
                  : (state == State::UpToDate) ? "up_to_date"
                  : (state == State::UpdateAvailable) ? "update_available"
                  : (state == State::Downloading) ? "downloading"
                  : (state == State::Success) ? "success"
                  : "error";
    doc["busy"] = (state == State::Checking || state == State::Downloading);
    doc["current"] = current;
    doc["latest"] = latest;
    doc["asset"] = asset;
    doc["bytesTotal"] = total;
    doc["bytesDone"] = done;
    doc["progress"] = progress;
    if (error.length() > 0)
    {
        doc["error"] = error;
    }

    String out;
    serializeJson(doc, out);
    return out;
}

void GitHubOtaUpdater::checkTask(void *param)
{
    GitHubOtaUpdater *self = static_cast<GitHubOtaUpdater *>(param);
    self->doCheck();
    self->clearTask();
    vTaskDelete(nullptr);
}

void GitHubOtaUpdater::downloadTask(void *param)
{
    GitHubOtaUpdater *self = static_cast<GitHubOtaUpdater *>(param);
    self->doDownload();
    self->clearTask();
    vTaskDelete(nullptr);
}

void GitHubOtaUpdater::doCheck()
{
    if (!_networkConnected || !_networkConnected())
    {
        setState(State::Error, "Network not connected");
        return;
    }

    const String url = "https://api.github.com/repos/" + _owner + "/" + _repo + "/releases/latest";

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(kHttpTimeoutMs);

    HTTPClient http;
    http.useHTTP10(true);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(kHttpTimeoutMs);
    http.setUserAgent(kUserAgent);
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");

    if (!http.begin(client, url))
    {
        setState(State::Error, "Failed to connect to GitHub");
        return;
    }

    const int httpCode = http.GET();
    if (httpCode != 200)
    {
        http.end();
        setState(State::Error, "GitHub HTTP " + String(httpCode));
        return;
    }

    const String payload = http.getString();
    http.end();

    if (payload.length() == 0)
    {
        setState(State::Error, "Release response empty");
        return;
    }

    JsonDocument filter;
    filter["tag_name"] = true;
    filter["name"] = true;
    JsonObject assetFilter = filter["assets"].to<JsonArray>().add<JsonObject>();
    assetFilter["name"] = true;
    assetFilter["browser_download_url"] = true;

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (error)
    {
        String details = "Release JSON parse error: ";
        details += error.c_str();
        if (!payload.startsWith("{"))
        {
            details += " / starts with: ";
            details += payload.substring(0, 48);
        }
        setState(State::Error, details);
        return;
    }

    const String tag = doc["tag_name"] | doc["name"] | "";
    const String latestVersion = normalizeVersion(tag);
    if (!latestVersion.length())
    {
        setState(State::Error, "Missing release version");
        return;
    }

    lock();
    _latestVersion = latestVersion;
    unlock();

    if (compareVersions(latestVersion, _currentVersion) <= 0)
    {
        setState(State::UpToDate, "");
        return;
    }

    const JsonArray assets = doc["assets"].as<JsonArray>();
    const String matchToken = "_" + _buildVariant + "_V";
    for (JsonVariantConst asset : assets)
    {
        const String name = asset["name"] | "";
        if (!name.length())
        {
            continue;
        }
        if (!name.endsWith(".bin.ota"))
        {
            continue;
        }
        if (name.indexOf(matchToken) < 0)
        {
            continue;
        }

        const String assetUrl = asset["browser_download_url"] | "";
        if (!assetUrl.length())
        {
            continue;
        }

        lock();
        _assetName = name;
        _assetUrl = assetUrl;
        unlock();

        setState(State::UpdateAvailable, "");
        return;
    }

    setState(State::Error, "No matching asset for build");
}

void GitHubOtaUpdater::doDownload()
{
    if (!_networkConnected || !_networkConnected())
    {
        setState(State::Error, "Network not connected");
        return;
    }

    String assetUrl;
    lock();
    assetUrl = _assetUrl;
    unlock();

    if (!assetUrl.length())
    {
        setState(State::Error, "Missing asset URL");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(kHttpTimeoutMs);
    http.setUserAgent(kUserAgent);

    if (!http.begin(client, assetUrl))
    {
        setState(State::Error, "Failed to connect to asset");
        return;
    }

    const int httpCode = http.GET();
    if (httpCode != 200)
    {
        http.end();
        setState(State::Error, "Asset HTTP " + String(httpCode));
        return;
    }

    const int total = http.getSize();
    lock();
    _bytesTotal = (total > 0) ? static_cast<uint32_t>(total) : 0;
    _bytesDone = 0;
    unlock();

    if (!Update.begin(total > 0 ? static_cast<uint32_t>(total) : UPDATE_SIZE_UNKNOWN))
    {
        http.end();
        setState(State::Error, "Update begin failed");
        return;
    }

    auto *stream = http.getStreamPtr();
    uint8_t buffer[1024];
    int remaining = total;

    while (http.connected() && (remaining > 0 || remaining == -1))
    {
        const size_t available = stream->available();
        if (available == 0)
        {
            vTaskDelay(1);
            continue;
        }

        const size_t chunkSize = (available > sizeof(buffer)) ? sizeof(buffer) : available;
        const size_t readLen = stream->readBytes(buffer, chunkSize);
        if (readLen == 0)
        {
            vTaskDelay(1);
            continue;
        }

        if (Update.write(buffer, readLen) != readLen)
        {
            Update.abort();
            http.end();
            setState(State::Error, "Update write failed");
            return;
        }

        lock();
        _bytesDone += static_cast<uint32_t>(readLen);
        unlock();

        if (remaining > 0)
        {
            remaining -= static_cast<int>(readLen);
        }

        vTaskDelay(1);
    }

    http.end();

    if (!Update.end(true))
    {
        setState(State::Error, "Update finalize failed");
        return;
    }

    lock();
    if (_bytesTotal == 0)
    {
        _bytesTotal = _bytesDone;
    }
    else
    {
        _bytesDone = _bytesTotal;
    }
    unlock();

    setState(State::Success, "");
    scheduleRestart(2500);
}

void GitHubOtaUpdater::scheduleRestart(uint32_t delayMs)
{
    esp_timer_handle_t timer = nullptr;
    esp_timer_create_args_t args = {};
    args.callback = [](void *) { ESP.restart(); };
    args.arg = nullptr;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "solar_ota_restart";

    if (esp_timer_create(&args, &timer) == ESP_OK && timer != nullptr)
    {
        esp_timer_start_once(timer, static_cast<uint64_t>(delayMs) * 1000ULL);
    }
    else
    {
        ESP.restart();
    }
}

void GitHubOtaUpdater::setState(State state, const String &error)
{
    lock();
    _state = state;
    _stateTs = millis();
    _lastError = error;
    unlock();
}

void GitHubOtaUpdater::clearTask()
{
    lock();
    _task = nullptr;
    unlock();
}

String GitHubOtaUpdater::normalizeVersion(const String &value)
{
    String normalized = value;
    normalized.trim();
    if (normalized.length() > 0 && (normalized[0] == 'v' || normalized[0] == 'V'))
    {
        normalized = normalized.substring(1);
    }
    return normalized;
}

int GitHubOtaUpdater::compareVersions(const String &left, const String &right)
{
    int leftIndex = 0;
    int rightIndex = 0;
    const String normalizedLeft = normalizeVersion(left);
    const String normalizedRight = normalizeVersion(right);

    while (leftIndex < normalizedLeft.length() || rightIndex < normalizedRight.length())
    {
        long leftNumber = 0;
        long rightNumber = 0;

        while (leftIndex < normalizedLeft.length() && !isDigit(normalizedLeft[leftIndex]))
        {
            leftIndex++;
        }
        while (rightIndex < normalizedRight.length() && !isDigit(normalizedRight[rightIndex]))
        {
            rightIndex++;
        }
        while (leftIndex < normalizedLeft.length() && isDigit(normalizedLeft[leftIndex]))
        {
            leftNumber = leftNumber * 10 + (normalizedLeft[leftIndex] - '0');
            leftIndex++;
        }
        while (rightIndex < normalizedRight.length() && isDigit(normalizedRight[rightIndex]))
        {
            rightNumber = rightNumber * 10 + (normalizedRight[rightIndex] - '0');
            rightIndex++;
        }

        if (leftNumber < rightNumber)
        {
            return -1;
        }
        if (leftNumber > rightNumber)
        {
            return 1;
        }
    }

    return 0;
}

void GitHubOtaUpdater::lock() const
{
    if (_lockHandle != nullptr)
    {
        xSemaphoreTake(static_cast<SemaphoreHandle_t>(_lockHandle), portMAX_DELAY);
    }
}

void GitHubOtaUpdater::unlock() const
{
    if (_lockHandle != nullptr)
    {
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(_lockHandle));
    }
}
