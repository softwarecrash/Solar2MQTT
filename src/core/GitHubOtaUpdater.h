#pragma once

#include <Arduino.h>
#include <functional>

class GitHubOtaUpdater
{
public:
    enum class State : uint8_t
    {
        Idle,
        Checking,
        UpToDate,
        UpdateAvailable,
        Downloading,
        Success,
        Error
    };

    GitHubOtaUpdater(const char *owner,
                     const char *repo,
                     const char *currentVersion,
                     const char *buildVariant);

    void begin(std::function<bool()> networkConnected);
    bool requestCheck();
    bool startUpdate();
    String statusJson() const;
    bool isBusy() const;
    // Large status buffers and TLS must not be allocated concurrently. Taking
    // this guard never waits: WebSocket connect callbacks already hold a lock.
    class StatusGuard
    {
    public:
        explicit StatusGuard(GitHubOtaUpdater &updater);
        ~StatusGuard();
        explicit operator bool() const { return _owner != nullptr; }
        StatusGuard(const StatusGuard &) = delete;
        StatusGuard &operator=(const StatusGuard &) = delete;
    private:
        GitHubOtaUpdater *_owner = nullptr;
    };
    // Consumed by the main loop so energy is flushed before the ESP restarts.
    bool consumeRestartRequest(uint32_t nowMs);

private:
    static void checkTask(void *param);
    static void downloadTask(void *param);

    void doCheck();
    void doDownload();
    void scheduleRestart(uint32_t delayMs);
    void setState(State state, const String &error = "");
    void clearTask();

    static bool isValidVersion(const String &value);
    static int compareVersions(const String &left, const String &right);
    static String normalizeVersion(const String &value);

    void lock() const;
    void unlock() const;

    String _owner;
    String _repo;
    String _currentVersion;
    String _buildVariant;

    String _latestVersion;
    String _assetUrl;
    String _assetName;
    String _lastError;

    uint32_t _bytesTotal = 0;
    uint32_t _bytesDone = 0;
    uint32_t _stateTs = 0;
    State _state = State::Idle;

    void *_lockHandle = nullptr;
    void *_task = nullptr;
    bool _restartPending = false;
    uint32_t _restartAtMs = 0;
    std::function<bool()> _networkConnected;
};
