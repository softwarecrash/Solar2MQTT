#pragma once
#include <Arduino.h>
#include <algorithm>
#include <map>
#include <memory>
#include <limits>
#include <string>
#include <vector>

namespace battery_test
{
struct Storage
{
    bool partitionPresent = true, nvsPresent = true, mounted = false, mountFails = false,
         erased = false;
    bool flashReadFails = false, formatSucceeds = true, nvsAvailable = true;
    bool failFsWrites = false, failNvsWrites = false, failBoolWrites = false, failRemoves = false;
    size_t fileWriteLimit = std::numeric_limits<size_t>::max(),
           nvsWriteLimit = std::numeric_limits<size_t>::max();
    unsigned failFsReads = 0, failNvsReads = 0, formatCalls = 0, implicitFormatCalls = 0,
             nvsWrites = 0, fsWrites = 0;
    std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> files;
    std::map<std::string, std::vector<uint8_t>> blobs;
    std::map<std::string, bool> flags;
};
extern Storage storage;
} // namespace battery_test

constexpr int SeekSet = 0;
class File
{
    std::shared_ptr<std::vector<uint8_t>> data;
    size_t cursor = 0;

  public:
    File() = default;
    explicit File(std::shared_ptr<std::vector<uint8_t>> bytes, size_t start = 0)
        : data(bytes), cursor(start)
    {
    }
    explicit operator bool() const
    {
        return bool(data);
    }
    size_t size() const
    {
        return data ? data->size() : 0;
    }
    size_t read(uint8_t *target, size_t length)
    {
        if (!data)
            return 0;
        if (battery_test::storage.failFsReads)
        {
            --battery_test::storage.failFsReads;
            return 0;
        }
        const size_t take = std::min(length, data->size() - cursor);
        if (take)
            std::copy_n(data->data() + cursor, take, target);
        cursor += take;
        return take;
    }
    size_t write(const uint8_t *source, size_t length)
    {
        auto &s = battery_test::storage;
        ++s.fsWrites;
        if (!data || s.failFsWrites)
            return 0;
        const size_t take = std::min(length, s.fileWriteLimit);
        if (cursor + take > data->size())
            data->resize(cursor + take);
        if (take)
            std::copy_n(source, take, data->data() + cursor);
        cursor += take;
        return take;
    }
    bool seek(size_t offset, int)
    {
        if (!data || offset > data->size())
            return false;
        cursor = offset;
        return true;
    }
    void flush() {}
    void close()
    {
        data.reset();
    }
};
class FakeLittleFS
{
  public:
    bool begin(bool formatOnFail, const char *, unsigned, const char *name)
    {
        auto &s = battery_test::storage;
        if (!s.partitionPresent || std::string(name) != "backlog")
        {
            s.mounted = false;
            return false;
        }
        if (s.mountFails)
        {
            if (!formatOnFail)
            {
                s.mounted = false;
                return false;
            }
            ++s.implicitFormatCalls;
            if (!format())
            {
                s.mounted = false;
                return false;
            }
        }
        s.mounted = true;
        return true;
    }
    bool format()
    {
        auto &s = battery_test::storage;
        ++s.formatCalls;
        if (!s.partitionPresent || !s.formatSucceeds)
            return false;
        s.files.clear();
        s.mountFails = false;
        s.erased = false;
        return true;
    }
    File open(const char *name, const char *mode)
    {
        auto &s = battery_test::storage;
        if (!s.mounted)
            return File();
        const std::string key = name;
        if (mode[0] == 'w')
        {
            auto bytes = std::make_shared<std::vector<uint8_t>>();
            s.files[key] = bytes;
            return File(bytes);
        }
        auto found = s.files.find(key);
        if (mode[0] == 'a')
        {
            if (found == s.files.end())
            {
                auto bytes = std::make_shared<std::vector<uint8_t>>();
                s.files[key] = bytes;
                return File(bytes);
            }
            return File(found->second, found->second->size());
        }
        return found == s.files.end() ? File() : File(found->second);
    }
    bool exists(const char *name)
    {
        return battery_test::storage.mounted && battery_test::storage.files.count(name) > 0;
    }
    bool remove(const char *name)
    {
        auto &s = battery_test::storage;
        return s.mounted && !s.failRemoves && s.files.erase(name) > 0;
    }
    bool rename(const char *from, const char *to)
    {
        auto &s = battery_test::storage;
        const auto found = s.files.find(from);
        if (!s.mounted || found == s.files.end())
            return false;
        s.files[to] = found->second;
        s.files.erase(found);
        return true;
    }
};
extern FakeLittleFS LittleFS;
class Preferences
{
    std::string space;
    std::string key(const char *name) const
    {
        return space + ":" + name;
    }

  public:
    bool begin(const char *name, bool)
    {
        space = name;
        return battery_test::storage.nvsPresent && battery_test::storage.nvsAvailable;
    }
    bool getBool(const char *name, bool fallback)
    {
        const auto &m = battery_test::storage.flags;
        const auto found = m.find(key(name));
        return found == m.end() ? fallback : found->second;
    }
    size_t putBool(const char *name, bool value)
    {
        if (battery_test::storage.failBoolWrites)
            return 0;
        battery_test::storage.flags[key(name)] = value;
        return 1;
    }
    bool isKey(const char *name)
    {
        const auto &s = battery_test::storage;
        return s.blobs.count(key(name)) || s.flags.count(key(name));
    }
    size_t getBytesLength(const char *name)
    {
        const auto &m = battery_test::storage.blobs;
        const auto found = m.find(key(name));
        return found == m.end() ? 0 : found->second.size();
    }
    size_t getBytes(const char *name, void *target, size_t length)
    {
        auto &s = battery_test::storage;
        if (s.failNvsReads)
        {
            --s.failNvsReads;
            return 0;
        }
        const auto found = s.blobs.find(key(name));
        if (found == s.blobs.end() || found->second.size() > length)
            return 0;
        const size_t n = found->second.size();
        if (n)
            std::copy_n(found->second.data(), n, static_cast<uint8_t *>(target));
        return n;
    }
    size_t putBytes(const char *name, const void *source, size_t length)
    {
        auto &s = battery_test::storage;
        ++s.nvsWrites;
        if (s.failNvsWrites)
            return 0;
        const size_t n = std::min(length, s.nvsWriteLimit);
        const auto *p = static_cast<const uint8_t *>(source);
        s.blobs[key(name)] = std::vector<uint8_t>(p, p + n);
        return n;
    }
    bool remove(const char *name)
    {
        auto &s = battery_test::storage;
        if (s.failRemoves)
            return false;
        return s.blobs.erase(key(name)) + s.flags.erase(key(name)) > 0;
    }
    void end() {}
};

using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
constexpr int ESP_PARTITION_TYPE_APP = 0, ESP_PARTITION_TYPE_DATA = 1,
              ESP_PARTITION_SUBTYPE_ANY = 255;
struct esp_partition_t
{
    int type, subtype;
    uint32_t address, size;
    char label[17];
};
inline const esp_partition_t *esp_partition_find_first(int, int, const char *name)
{
    static const esp_partition_t fs = {1, 0x83, 0x390000, 0x60000, "backlog"};
    static const esp_partition_t nvs = {1, 2, 0x9000, 0x5000, "nvs"};
    if (std::strcmp(name, "backlog") == 0)
        return battery_test::storage.partitionPresent ? &fs : nullptr;
    if (std::strcmp(name, "nvs") == 0)
        return battery_test::storage.nvsPresent ? &nvs : nullptr;
    return nullptr;
}
inline const esp_partition_t *esp_ota_get_running_partition()
{
    static const esp_partition_t app = {0, 0x10, 0x10000, 0x1f0000, "app0"};
    return &app;
}
inline const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *)
{
    static const esp_partition_t app = {0, 0x11, 0x200000, 0x1f0000, "app1"};
    return &app;
}
inline esp_err_t esp_partition_read(const esp_partition_t *, size_t, void *target, size_t length)
{
    if (battery_test::storage.flashReadFails)
        return -1;
    std::memset(target, battery_test::storage.erased ? 0xff : 0, length);
    return ESP_OK;
}
