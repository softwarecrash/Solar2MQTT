#include "core/GitHubOtaUpdater.h"
#include "core/FirmwareWriter.h"

FirmwareWriter firmwareWriter;

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <cstring>
#include <freertos/task.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace
{
constexpr uint32_t kOtaTaskStack = 8192;
constexpr uint32_t kHttpTimeoutMs = 15000;
constexpr const char *kUserAgent = "Solar2MQTT-OTA";
constexpr uint32_t kReleaseRetryDelayMs = 1000;

// ESP32 TLS readBytes() can return zero between packets while connected.
// ArduinoJson treats that as EOF, so wait for the next byte with a bounded,
// wrap-safe inactivity timeout instead of buffering the whole release body.
class ReleaseResponseReader
{
public:
    ReleaseResponseReader(HTTPClient &http, NetworkClient &stream) : _http(http), _stream(stream) {}
    int read()
    {
        const uint32_t startedAt = millis();
        for (;;)
        {
            const int value = _stream.read();
            if (value >= 0)
                return value;
            if (!_http.connected() || static_cast<uint32_t>(millis() - startedAt) >= kHttpTimeoutMs)
                return -1;
            delay(1);
        }
    }
    size_t readBytes(char *buffer, size_t length)
    {
        size_t count = 0;
        while (count < length)
        {
            const int value = read();
            if (value < 0)
                break;
            buffer[count++] = static_cast<char>(value);
        }
        return count;
    }
private:
    HTTPClient &_http;
    NetworkClient &_stream;
};

// Compare numeric identifiers by digit count, avoiding integer overflow.
struct VersionRange
{
    const char *begin = nullptr;
    const char *end = nullptr;
};
struct VersionParts
{
    VersionRange core[3];
    VersionRange prerelease;
    VersionRange diagnostic;
};
bool versionDigit(char c)
{
    return c >= '0' && c <= '9';
}
bool versionLetter(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool versionIdentifiers(const char *&cursor, VersionRange &range, bool prerelease)
{
    range.begin = cursor;
    do
    {
        const char *start = cursor;
        bool numeric = true;
        while (versionDigit(*cursor) || versionLetter(*cursor) || *cursor == '-')
        {
            numeric = numeric && versionDigit(*cursor);
            ++cursor;
        }
        if (cursor == start || (prerelease && numeric && cursor - start > 1 && *start == '0'))
            return false;
        if (*cursor != '.')
            break;
        ++cursor;
    } while (true);
    range.end = cursor;
    return true;
}
bool parseVersion(const char *cursor, VersionParts &parts)
{
    for (size_t i = 0; i < 3; ++i)
    {
        parts.core[i].begin = cursor;
        while (versionDigit(*cursor))
            ++cursor;
        parts.core[i].end = cursor;
        if (cursor == parts.core[i].begin ||
            (cursor - parts.core[i].begin > 1 && *parts.core[i].begin == '0'))
            return false;
        if (i < 2 && *cursor++ != '.')
            return false;
    }
    // Preserve the repository's diagnostic release order: 2.0.14 < 2.0.14A.
    if (versionLetter(*cursor))
    {
        parts.diagnostic.begin = cursor;
        while (versionLetter(*cursor))
            ++cursor;
        parts.diagnostic.end = cursor;
    }
    else if (*cursor == '-')
    {
        ++cursor;
        if (!versionIdentifiers(cursor, parts.prerelease, true))
            return false;
    }
    if (*cursor == '+')
    {
        ++cursor;
        VersionRange metadata;
        if (!versionIdentifiers(cursor, metadata, false))
            return false;
    }
    return *cursor == '\0';
}
int compareRange(VersionRange left, VersionRange right, bool numeric, bool ignoreCase = false)
{
    const size_t leftSize = left.end - left.begin, rightSize = right.end - right.begin;
    if (numeric && leftSize != rightSize)
        return leftSize < rightSize ? -1 : 1;
    for (size_t i = 0; i < leftSize && i < rightSize; ++i)
    {
        char a = left.begin[i], b = right.begin[i];
        if (ignoreCase)
        {
            if (a >= 'a' && a <= 'z')
                a -= 'a' - 'A';
            if (b >= 'a' && b <= 'z')
                b -= 'a' - 'A';
        }
        if (a != b)
            return a < b ? -1 : 1;
    }
    return leftSize == rightSize ? 0 : (leftSize < rightSize ? -1 : 1);
}

} // namespace

GitHubOtaUpdater::GitHubOtaUpdater(const char *owner, const char *repo, const char *currentVersion,
                                   const char *buildVariant)
    : _owner(owner ? owner : ""), _repo(repo ? repo : ""),
      _currentVersion(currentVersion ? currentVersion : ""),
      _buildVariant(buildVariant ? buildVariant : "")
{
}

void GitHubOtaUpdater::begin(std::function<bool()> networkConnected)
{
    firmwareWriter.begin();
    _networkConnected = std::move(networkConnected);
    if (_lockHandle == nullptr)
    {
        _lockHandle = xSemaphoreCreateMutex();
        if (_lockHandle == nullptr)
            setState(State::Error, "Failed to create update lock");
    }
}

bool GitHubOtaUpdater::isBusy() const
{
    lock();
    const bool busy =
        (_state == State::Checking || _state == State::Downloading || _task != nullptr);
    unlock();
    return busy;
}

GitHubOtaUpdater::StatusGuard::StatusGuard(GitHubOtaUpdater &updater)
{
    if (updater._lockHandle == nullptr ||
        xSemaphoreTake(static_cast<SemaphoreHandle_t>(updater._lockHandle), 0) != pdTRUE)
        return;
    if (updater._task != nullptr || updater._state == State::Checking ||
        updater._state == State::Downloading || updater._restartPending)
    {
        updater.unlock();
        return;
    }
    // requestCheck/startUpdate use this same lock before starting their worker.
    // Retain it until the status document and send buffers have been created.
    _owner = &updater;
}

GitHubOtaUpdater::StatusGuard::~StatusGuard()
{
    if (_owner != nullptr)
        _owner->unlock();
}

bool GitHubOtaUpdater::requestCheck()
{
    if (_lockHandle == nullptr)
        return false;
    lock();
    if (_task != nullptr || _state == State::Checking || _state == State::Downloading ||
        _restartPending)
    {
        unlock();
        return false;
    }
    _bytesDone = _bytesTotal = 0;
    _latestVersion = _assetUrl = _assetName = _lastError = "";
    _state = State::Checking;
    _stateTs = millis();
    TaskHandle_t handle = nullptr;
    // Keep ownership locked until the handle is stored: a fast task must not
    // clear itself before the creating request has registered it.
    const bool started =
        xTaskCreate(checkTask, "solar_ota_check", kOtaTaskStack, this, 1, &handle) == pdPASS;
    if (started)
        _task = handle;
    else
    {
        _state = State::Error;
        _lastError = "Failed to start update check task";
    }
    unlock();
    return started;
}

bool GitHubOtaUpdater::startUpdate()
{
    if (_lockHandle == nullptr)
        return false;
    lock();
    if (_state != State::UpdateAvailable || _task != nullptr || _restartPending)
    {
        unlock();
        return false;
    }
    _bytesDone = _bytesTotal = 0;
    _lastError = "";
    _state = State::Downloading;
    _stateTs = millis();
    TaskHandle_t handle = nullptr;
    const bool started =
        xTaskCreate(downloadTask, "solar_ota_dl", kOtaTaskStack, this, 1, &handle) == pdPASS;
    if (started)
        _task = handle;
    else
    {
        _state = State::Error;
        _lastError = "Failed to start update task";
    }
    unlock();
    return started;
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
    bool busy = false;

    lock();
    current = _currentVersion;
    latest = _latestVersion;
    asset = _assetName;
    error = _lastError;
    total = _bytesTotal;
    done = _bytesDone;
    state = _state;
    busy = _task != nullptr || state == State::Checking || state == State::Downloading;
    unlock();

    const uint8_t progress = (total > 0) ? static_cast<uint8_t>((done * 100ULL) / total) : 0;

    JsonDocument doc;
    doc["state"] = (state == State::Idle)              ? "idle"
                   : (state == State::Checking)        ? "checking"
                   : (state == State::UpToDate)        ? "up_to_date"
                   : (state == State::UpdateAvailable) ? "update_available"
                   : (state == State::Downloading)     ? "downloading"
                   : (state == State::Success)         ? "success"
                                                       : "error";
    doc["busy"] = busy;
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
    client.setHandshakeTimeout(kHttpTimeoutMs / 1000);
    client.setTimeout(kHttpTimeoutMs);

    HTTPClient http;
    http.useHTTP10(true);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setConnectTimeout(kHttpTimeoutMs);
    http.setTimeout(kHttpTimeoutMs);
    http.setUserAgent(kUserAgent);
    auto beginReleaseRequest = [&]() {
        if (!http.begin(client, url))
            return false;
        http.addHeader("Accept", "application/vnd.github+json");
        http.addHeader("X-GitHub-Api-Version", "2022-11-28");
        return true;
    };
    if (!beginReleaseRequest())
    {
        setState(State::Error, "Failed to connect to GitHub");
        return;
    }

    int httpCode = http.GET();
    // A metadata GET is safe to reconnect once after a transient transport
    // failure. Release TLS resources first; never retry firmware writes here.
    if (httpCode < 0)
    {
        http.end();
        client.stop();
        delay(kReleaseRetryDelayMs);
        if (!beginReleaseRequest())
        {
            setState(State::Error, "Failed to reconnect to GitHub");
            return;
        }
        httpCode = http.GET();
    }
    if (httpCode != 200)
    {
        http.end();
        setState(State::Error, "GitHub HTTP " + String(httpCode));
        return;
    }

    JsonDocument filter;
    filter["tag_name"] = true;
    filter["name"] = true;
    filter["draft"] = true;
    filter["prerelease"] = true;
    JsonObject assetFilter = filter["assets"].to<JsonArray>().add<JsonObject>();
    assetFilter["name"] = true;
    assetFilter["browser_download_url"] = true;
    if (filter.overflowed())
    {
        http.end();
        setState(State::Error, "Not enough memory for release filter");
        return;
    }

    JsonDocument doc;
    // HTTP/1.0 avoids chunked framing. Filter directly from the network stream:
    // getString() would need another contiguous buffer for the whole response.
    auto *stream = http.getStreamPtr();
    if (stream == nullptr)
    {
        http.end();
        setState(State::Error, "Release stream unavailable");
        return;
    }
    ReleaseResponseReader reader(http, *stream);
    const DeserializationError error =
        deserializeJson(doc, reader, DeserializationOption::Filter(filter));
    http.end();
    if (error)
    {
        String details = "Release JSON parse error: ";
        details += error.c_str();
        setState(State::Error, details);
        return;
    }

    if ((doc["draft"] | false) || (doc["prerelease"] | false))
    {
        setState(State::Error, "GitHub response is not a stable release");
        return;
    }
    const String tag = doc["tag_name"] | doc["name"] | "";
    const String latestVersion = normalizeVersion(tag);
    if (!isValidVersion(latestVersion) || !isValidVersion(normalizeVersion(_currentVersion)))
    {
        setState(State::Error, "Missing or invalid firmware version");
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
    const String matchToken = "_" + _buildVariant + "_V" + latestVersion + ".bin.ota";
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
        if (!name.endsWith(matchToken.c_str()))
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
    client.setHandshakeTimeout(kHttpTimeoutMs / 1000);

    HTTPClient http;
    http.useHTTP10(true);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setConnectTimeout(kHttpTimeoutMs);
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
    auto *stream = http.getStreamPtr();
    if (total == 0 || stream == nullptr)
    {
        http.end();
        setState(State::Error, "Empty firmware response");
        return;
    }
    lock();
    _bytesTotal = (total > 0) ? static_cast<uint32_t>(total) : 0;
    _bytesDone = 0;
    unlock();

    FirmwareWriterLease writer(this);
    if (!writer.acquired)
    {
        http.end();
        setState(State::Error, "Another firmware update is in progress");
        return;
    }
    if (!Update.begin(total > 0 ? static_cast<uint32_t>(total) : UPDATE_SIZE_UNKNOWN))
    {
        http.end();
        setState(State::Error, "Update begin failed");
        return;
    }

    uint8_t buffer[1024];
    int remaining = total;
    uint32_t lastProgressAt = millis();

    while ((http.connected() || stream->available() > 0) && (remaining > 0 || remaining == -1))
    {
        if (static_cast<uint32_t>(millis() - lastProgressAt) >= kHttpTimeoutMs)
        {
            Update.abort();
            http.end();
            setState(State::Error, "Firmware download timed out");
            return;
        }
        const size_t available = stream->available();
        if (available == 0)
        {
            vTaskDelay(1);
            continue;
        }

        size_t chunkSize = (available > sizeof(buffer)) ? sizeof(buffer) : available;
        if (remaining > 0 && chunkSize > static_cast<size_t>(remaining))
            chunkSize = remaining;
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
        lastProgressAt = millis();

        vTaskDelay(1);
    }

    http.end();

    if (remaining > 0 || _bytesDone == 0)
    {
        Update.abort();
        setState(State::Error, "Incomplete firmware download");
        return;
    }
    if (!Update.end(total < 0))
    {
        setState(State::Error, "Update finalize failed");
        return;
    }
    writer.commit();

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
    lock();
    _restartAtMs = static_cast<uint32_t>(millis()) + delayMs;
    _restartPending = true;
    unlock();
}

bool GitHubOtaUpdater::consumeRestartRequest(uint32_t nowMs)
{
    lock();
    const bool ready = _restartPending && static_cast<int32_t>(nowMs - _restartAtMs) >= 0;
    if (ready)
        _restartPending = false;
    unlock();
    return ready;
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
    String version = value;
    version.trim();
    if (version.startsWith("v") || version.startsWith("V"))
        version.remove(0, 1);
    return version;
}

int GitHubOtaUpdater::compareVersions(const String &left, const String &right)
{
    const String normalizedLeft = normalizeVersion(left), normalizedRight = normalizeVersion(right);
    VersionParts a, b;
    if (!parseVersion(normalizedLeft.c_str(), a) || !parseVersion(normalizedRight.c_str(), b))
        return 0;
    for (size_t i = 0; i < 3; ++i)
    {
        const int result = compareRange(a.core[i], b.core[i], true);
        if (result != 0)
            return result;
    }
    if (a.prerelease.begin != nullptr || b.prerelease.begin != nullptr)
    {
        if (a.prerelease.begin == nullptr)
            return 1;
        if (b.prerelease.begin == nullptr)
            return -1;
        const char *x = a.prerelease.begin, *y = b.prerelease.begin;
        while (x < a.prerelease.end && y < b.prerelease.end)
        {
            VersionRange ax{x, x}, by{y, y};
            bool nx = true, ny = true;
            while (ax.end < a.prerelease.end && *ax.end != '.')
            {
                nx = nx && versionDigit(*ax.end);
                ++ax.end;
            }
            while (by.end < b.prerelease.end && *by.end != '.')
            {
                ny = ny && versionDigit(*by.end);
                ++by.end;
            }
            if (nx != ny)
                return nx ? -1 : 1;
            const int result = compareRange(ax, by, nx);
            if (result != 0)
                return result;
            x = ax.end;
            y = by.end;
            if (x < a.prerelease.end)
                ++x;
            if (y < b.prerelease.end)
                ++y;
        }
        if (x != a.prerelease.end || y != b.prerelease.end)
            return x == a.prerelease.end ? -1 : 1;
    }
    if (a.diagnostic.begin == nullptr || b.diagnostic.begin == nullptr)
        return a.diagnostic.begin == b.diagnostic.begin ? 0
                                                        : (a.diagnostic.begin == nullptr ? -1 : 1);
    return compareRange(a.diagnostic, b.diagnostic, false, true);
}

bool GitHubOtaUpdater::isValidVersion(const String &value)
{
    VersionParts parts;
    return parseVersion(value.c_str(), parts);
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
