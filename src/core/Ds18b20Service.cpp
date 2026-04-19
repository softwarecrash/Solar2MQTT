#include "core/Ds18b20Service.h"

#include "core/LogSerial.h"

#include <algorithm>
#include <math.h>

namespace
{
constexpr size_t kMaxSensors = 15;
constexpr uint32_t kExpirationSeconds = 5;
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kDiscoveryRetryMs = 10000;
constexpr uint8_t kSearchAttempts = 25;
constexpr uint32_t kSearchRetryDelayMs = 20;
constexpr uint8_t kSensorRecoveryThreshold = 6;
constexpr uint8_t kBusRecoveryThreshold = 8;

bool nearlyEqual(float lhs, float rhs)
{
    return fabsf(lhs - rhs) < 0.01f;
}

bool timeReached(uint32_t now, uint32_t scheduledAt)
{
    return static_cast<int32_t>(now - scheduledAt) >= 0;
}
} // namespace

Ds18b20Service::Ds18b20Service()
    : _pin(-1),
      _nextPollAt(0),
      _nextDiscoveryAt(0),
      _consecutiveFailedPolls(0),
      _enabled(false),
      _active(false),
      _sensorCount(0)
{
}

void Ds18b20Service::begin(uint8_t pin, JsonObject target)
{
    _target = target;
    _pin = static_cast<int8_t>(pin);
    _sensors.clear();
    _oneWire.reset();
    _nextPollAt = 0;
    _nextDiscoveryAt = 0;
    _consecutiveFailedPolls = 0;
    _sensorCount = 0;
    clearTargetRange(1);

    if (pin == static_cast<uint8_t>(-1))
    {
        _enabled = false;
        _active = false;
        LogSerial.println("[DS18B20] Disabled");
        return;
    }

    _enabled = true;
    discoverSensors();
}

bool Ds18b20Service::discoverSensors()
{
    _active = false;
    _sensorCount = 0;
    _consecutiveFailedPolls = 0;
    _nextPollAt = 0;
    _sensors.clear();
    _oneWire.reset();

    if (!_enabled || _pin < 0)
    {
        return false;
    }

    _oneWire = std::make_unique<OneWire32>(static_cast<uint8_t>(_pin));
    if (!_oneWire)
    {
        _nextDiscoveryAt = millis() + kDiscoveryRetryMs;
        LogSerial.println("[DS18B20] Failed to allocate OneWire32");
        return false;
    }

    uint64_t addresses[kMaxSensors] = {};
    size_t found = 0;
    for (uint8_t attempt = 0; attempt < kSearchAttempts && found == 0; ++attempt)
    {
        found = _oneWire->search(addresses, kMaxSensors);
        if (found == 0)
        {
            delay(kSearchRetryDelayMs);
        }
    }

    if (found > 1)
    {
        std::sort(addresses, addresses + found);
    }

    _sensors.reserve(found);
    for (size_t index = 0; index < found; ++index)
    {
        SensorSlot slot;
        slot.address = addresses[index];
        slot.sensor = std::make_unique<Mycila::DS18>();
        if (!slot.sensor)
        {
            continue;
        }

        slot.sensor->begin(_oneWire.get(), slot.address);
        slot.sensor->setExpirationDelay(kExpirationSeconds);
        _sensors.push_back(std::move(slot));
        LogSerial.printf("[DS18B20] Sensor %u address: %016llX\n",
                         static_cast<unsigned>(_sensors.size()),
                         static_cast<unsigned long long>(addresses[index]));
    }

    _sensorCount = _sensors.size();
    _active = _sensorCount > 0;

    LogSerial.printf("[DS18B20] Sensors found: %u\n", static_cast<unsigned>(_sensorCount));
    if (!_active)
    {
        clearTargetRange(1);
        _nextDiscoveryAt = millis() + kDiscoveryRetryMs;
        LogSerial.printf("[DS18B20] No sensors found on GPIO %d, retry in %lu ms\n",
                         static_cast<int>(_pin),
                         static_cast<unsigned long>(kDiscoveryRetryMs));
        return false;
    }

    clearTargetRange(_sensorCount + 1);
    _nextPollAt = millis() + kPollIntervalMs;
    _nextDiscoveryAt = 0;
    return true;
}

void Ds18b20Service::loop()
{
    if (!_enabled)
    {
        return;
    }

    const uint32_t now = millis();

    if (!_active)
    {
        if (_nextDiscoveryAt != 0 && timeReached(now, _nextDiscoveryAt))
        {
            LogSerial.printf("[DS18B20] Retrying discovery on GPIO %d\n", static_cast<int>(_pin));
            discoverSensors();
        }
        return;
    }

    if (!timeReached(now, _nextPollAt))
    {
        return;
    }
    _nextPollAt = now + kPollIntervalMs;

    bool anyFreshRead = false;
    for (size_t index = 0; index < _sensors.size(); ++index)
    {
        SensorSlot &slot = _sensors[index];
        if (!slot.sensor)
        {
            continue;
        }

        const bool freshRead = slot.sensor->read();
        if (freshRead)
        {
            anyFreshRead = true;
            slot.missedPolls = 0;
        }
        else if (slot.missedPolls < 0xFF)
        {
            ++slot.missedPolls;
        }

        const std::optional<float> temperature = slot.sensor->getTemperature();
        if (temperature.has_value())
        {
            if (!slot.hasValue || !nearlyEqual(slot.lastTemperature, *temperature))
            {
                slot.lastTemperature = *temperature;
                slot.hasValue = true;
                publishValue(index + 1, *temperature);
            }
            continue;
        }

        if (slot.hasValue)
        {
            slot.hasValue = false;
            clearValue(index + 1);
        }

        if (slot.missedPolls == kSensorRecoveryThreshold)
        {
            LogSerial.printf("[DS18B20] Sensor %u stopped updating, reinitializing address %016llX\n",
                             static_cast<unsigned>(index + 1),
                             static_cast<unsigned long long>(slot.address));
            slot.sensor->end();
            slot.sensor->begin(_oneWire.get(), slot.address);
            slot.sensor->setExpirationDelay(kExpirationSeconds);
            slot.missedPolls = 0;
        }
    }

    if (anyFreshRead)
    {
        _consecutiveFailedPolls = 0;
        return;
    }

    if (_consecutiveFailedPolls < 0xFF)
    {
        ++_consecutiveFailedPolls;
    }

    if (_consecutiveFailedPolls >= kBusRecoveryThreshold)
    {
        LogSerial.printf("[DS18B20] Bus recovery after %u failed polls on GPIO %d\n",
                         static_cast<unsigned>(_consecutiveFailedPolls),
                         static_cast<int>(_pin));
        discoverSensors();
    }
}

void Ds18b20Service::clearTargetRange(size_t fromIndex)
{
    if (_target.isNull())
    {
        return;
    }

    for (size_t index = fromIndex; index <= kMaxSensors; ++index)
    {
        char key[16];
        snprintf(key, sizeof(key), "DS18B20_%u", static_cast<unsigned>(index));
        _target.remove(key);
    }
}

void Ds18b20Service::publishValue(size_t index, float temperature)
{
    if (_target.isNull())
    {
        return;
    }

    char key[16];
    snprintf(key, sizeof(key), "DS18B20_%u", static_cast<unsigned>(index));
    _target[key] = temperature;
    if (_callback)
    {
        _callback(static_cast<uint8_t>(index), temperature);
    }
}

void Ds18b20Service::clearValue(size_t index)
{
    if (_target.isNull())
    {
        return;
    }

    char key[16];
    snprintf(key, sizeof(key), "DS18B20_%u", static_cast<unsigned>(index));
    _target[key] = nullptr;
}
