#include "core/SettingsPrefs.h"

#include <string.h>

namespace
{
void makeNvsKey(const char *group, const char *name, char out[16])
{
    if (!out)
    {
        return;
    }

    const size_t nameLength = name ? strlen(name) : 0;
    if (!name || nameLength == 0)
    {
        out[0] = '\0';
        return;
    }

    if (nameLength <= 15)
    {
        strncpy(out, name, 15);
        out[15] = '\0';
        return;
    }

    uint32_t hash = 2166136261u;
    if (group)
    {
        for (const uint8_t *p = reinterpret_cast<const uint8_t *>(group); *p; ++p)
        {
            hash ^= *p;
            hash *= 16777619u;
        }
    }

    hash ^= static_cast<uint8_t>('/');
    hash *= 16777619u;

    for (const uint8_t *p = reinterpret_cast<const uint8_t *>(name); *p; ++p)
    {
        hash ^= *p;
        hash *= 16777619u;
    }

    memset(out, 0, 16);
    for (size_t i = 0; i < 6 && name[i]; ++i)
    {
        out[i] = name[i];
    }
    out[6] = '_';

    constexpr char hexDigits[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; ++i)
    {
        out[7 + i] = hexDigits[(hash >> (28 - 4 * i)) & 0x0F];
    }
    out[15] = '\0';
}
} // namespace

SettingsGetter::SettingsGetter(Settings &outer) : _outer(outer) {}
SettingsSetter::SettingsSetter(Settings &outer) : _outer(outer) {}

Settings::Settings() : get(*this), set(*this), _initialized(false) {}

void Settings::begin()
{
    if (_initialized)
    {
        return;
    }

    _doc.clear();
    loadFromNvs();
    _initialized = true;
}

void Settings::ensureInit()
{
    if (!_initialized)
    {
        begin();
    }
}

void Settings::loadFromNvs()
{
    Preferences prefs;

#define LOAD_BOOL(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        _doc[group][name] = prefs.getBool(key, def); \
        prefs.end(); \
    }
#define LOAD_INT32(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        int32_t value = prefs.getInt(key, def); \
        prefs.end(); \
        if (value < (minv)) value = (minv); \
        if (value > (maxv)) value = (maxv); \
        _doc[group][name] = value; \
    }
#define LOAD_UINT16(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        uint16_t value = prefs.getUShort(key, static_cast<uint16_t>(def)); \
        prefs.end(); \
        if (value < static_cast<uint16_t>(minv)) value = static_cast<uint16_t>(minv); \
        if (value > static_cast<uint16_t>(maxv)) value = static_cast<uint16_t>(maxv); \
        _doc[group][name] = value; \
    }
#define LOAD_UINT32(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        uint32_t value = prefs.getUInt(key, static_cast<uint32_t>(def)); \
        prefs.end(); \
        if (value < static_cast<uint32_t>(minv)) value = static_cast<uint32_t>(minv); \
        if (value > static_cast<uint32_t>(maxv)) value = static_cast<uint32_t>(maxv); \
        _doc[group][name] = value; \
    }
#define LOAD_FLOAT(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        float value = prefs.getFloat(key, static_cast<float>(def)); \
        prefs.end(); \
        if (value < static_cast<float>(minv)) value = static_cast<float>(minv); \
        if (value > static_cast<float>(maxv)) value = static_cast<float>(maxv); \
        _doc[group][name] = value; \
    }
#define LOAD_STRING(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, true); \
        _doc[group][name] = prefs.getString(key, def); \
        prefs.end(); \
    }
#define SETTINGS_LOAD(type, group, name, api, def, minv, maxv) LOAD_##type(group, name, api, def, minv, maxv)
    SETTINGS_ITEMS(SETTINGS_LOAD)
#undef SETTINGS_LOAD
#undef LOAD_BOOL
#undef LOAD_INT32
#undef LOAD_UINT16
#undef LOAD_UINT32
#undef LOAD_FLOAT
#undef LOAD_STRING
}

void Settings::writeToNvs()
{
    Preferences prefs;

#define SAVE_BOOL(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        prefs.putBool(key, _doc[group][name] | def); \
        prefs.end(); \
    }
#define SAVE_INT32(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        int32_t value = _doc[group][name] | def; \
        if (value < (minv)) value = (minv); \
        if (value > (maxv)) value = (maxv); \
        prefs.putInt(key, value); \
        prefs.end(); \
    }
#define SAVE_UINT16(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        uint16_t value = static_cast<uint16_t>(_doc[group][name] | def); \
        if (value < static_cast<uint16_t>(minv)) value = static_cast<uint16_t>(minv); \
        if (value > static_cast<uint16_t>(maxv)) value = static_cast<uint16_t>(maxv); \
        prefs.putUShort(key, value); \
        prefs.end(); \
    }
#define SAVE_UINT32(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        uint32_t value = static_cast<uint32_t>(_doc[group][name] | def); \
        if (value < static_cast<uint32_t>(minv)) value = static_cast<uint32_t>(minv); \
        if (value > static_cast<uint32_t>(maxv)) value = static_cast<uint32_t>(maxv); \
        prefs.putUInt(key, value); \
        prefs.end(); \
    }
#define SAVE_FLOAT(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        float value = static_cast<float>(_doc[group][name] | def); \
        if (value < static_cast<float>(minv)) value = static_cast<float>(minv); \
        if (value > static_cast<float>(maxv)) value = static_cast<float>(maxv); \
        prefs.putFloat(key, value); \
        prefs.end(); \
    }
#define SAVE_STRING(group, name, api, def, minv, maxv) \
    { \
        char key[16]; \
        makeNvsKey(group, name, key); \
        prefs.begin(group, false); \
        prefs.putString(key, _doc[group][name] | def); \
        prefs.end(); \
    }
#define SETTINGS_SAVE(type, group, name, api, def, minv, maxv) SAVE_##type(group, name, api, def, minv, maxv)
    SETTINGS_ITEMS(SETTINGS_SAVE)
#undef SETTINGS_SAVE
#undef SAVE_BOOL
#undef SAVE_INT32
#undef SAVE_UINT16
#undef SAVE_UINT32
#undef SAVE_FLOAT
#undef SAVE_STRING
}

bool Settings::save()
{
    ensureInit();
    writeToNvs();
    return true;
}

String Settings::backup(bool pretty)
{
    ensureInit();
    String json;
    if (pretty)
    {
        serializeJsonPretty(_doc, json);
    }
    else
    {
        serializeJson(_doc, json);
    }
    return json;
}

bool Settings::restore(const String &json, bool merge, bool saveAfter)
{
    ensureInit();

    JsonDocument tmp;
    if (deserializeJson(tmp, json))
    {
        return false;
    }

    if (!merge)
    {
        _doc.clear();
    }

#define RESTORE_BOOL(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) _doc[group][name] = value.as<bool>(); \
    }
#define RESTORE_INT32(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) \
        { \
            int32_t out = value.as<int32_t>(); \
            if (out < (minv)) out = (minv); \
            if (out > (maxv)) out = (maxv); \
            _doc[group][name] = out; \
        } \
    }
#define RESTORE_UINT16(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) \
        { \
            uint16_t out = static_cast<uint16_t>(value.as<uint32_t>()); \
            if (out < static_cast<uint16_t>(minv)) out = static_cast<uint16_t>(minv); \
            if (out > static_cast<uint16_t>(maxv)) out = static_cast<uint16_t>(maxv); \
            _doc[group][name] = out; \
        } \
    }
#define RESTORE_UINT32(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) \
        { \
            uint32_t out = value.as<uint32_t>(); \
            if (out < static_cast<uint32_t>(minv)) out = static_cast<uint32_t>(minv); \
            if (out > static_cast<uint32_t>(maxv)) out = static_cast<uint32_t>(maxv); \
            _doc[group][name] = out; \
        } \
    }
#define RESTORE_FLOAT(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) \
        { \
            float out = value.as<float>(); \
            if (out < static_cast<float>(minv)) out = static_cast<float>(minv); \
            if (out > static_cast<float>(maxv)) out = static_cast<float>(maxv); \
            _doc[group][name] = out; \
        } \
    }
#define RESTORE_STRING(group, name, api, def, minv, maxv) \
    { \
        JsonVariant value = tmp[group][name]; \
        if (!value.isNull()) _doc[group][name] = value.as<String>(); \
    }
#define SETTINGS_RESTORE(type, group, name, api, def, minv, maxv) RESTORE_##type(group, name, api, def, minv, maxv)
    SETTINGS_ITEMS(SETTINGS_RESTORE)
#undef SETTINGS_RESTORE
#undef RESTORE_BOOL
#undef RESTORE_INT32
#undef RESTORE_UINT16
#undef RESTORE_UINT32
#undef RESTORE_FLOAT
#undef RESTORE_STRING

    if (saveAfter)
    {
        writeToNvs();
    }
    return true;
}

#define IMPL_GET_BOOL(group, name, api, def, minv, maxv) \
    bool SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        return value.is<bool>() ? value.as<bool>() : def; \
    }
#define IMPL_GET_INT32(group, name, api, def, minv, maxv) \
    int32_t SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        int32_t out = value.is<int32_t>() ? value.as<int32_t>() : static_cast<int32_t>(def); \
        if (out < (minv)) out = (minv); \
        if (out > (maxv)) out = (maxv); \
        return out; \
    }
#define IMPL_GET_UINT16(group, name, api, def, minv, maxv) \
    uint16_t SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        uint16_t out = value.is<uint32_t>() ? static_cast<uint16_t>(value.as<uint32_t>()) : static_cast<uint16_t>(def); \
        if (out < static_cast<uint16_t>(minv)) out = static_cast<uint16_t>(minv); \
        if (out > static_cast<uint16_t>(maxv)) out = static_cast<uint16_t>(maxv); \
        return out; \
    }
#define IMPL_GET_UINT32(group, name, api, def, minv, maxv) \
    uint32_t SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        uint32_t out = value.is<uint32_t>() ? value.as<uint32_t>() : static_cast<uint32_t>(def); \
        if (out < static_cast<uint32_t>(minv)) out = static_cast<uint32_t>(minv); \
        if (out > static_cast<uint32_t>(maxv)) out = static_cast<uint32_t>(maxv); \
        return out; \
    }
#define IMPL_GET_FLOAT(group, name, api, def, minv, maxv) \
    float SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        float out = value.is<float>() ? value.as<float>() : static_cast<float>(def); \
        if (out < static_cast<float>(minv)) out = static_cast<float>(minv); \
        if (out > static_cast<float>(maxv)) out = static_cast<float>(maxv); \
        return out; \
    }
#define IMPL_GET_STRING(group, name, api, def, minv, maxv) \
    const char *SettingsGetter::api() \
    { \
        _outer.ensureInit(); \
        JsonVariant value = _outer._doc[group][name]; \
        return value.isNull() ? def : value.as<const char *>(); \
    }
#define SETTINGS_IMPL_GET(type, group, name, api, def, minv, maxv) IMPL_GET_##type(group, name, api, def, minv, maxv)
SETTINGS_ITEMS(SETTINGS_IMPL_GET)
#undef SETTINGS_IMPL_GET
#undef IMPL_GET_BOOL
#undef IMPL_GET_INT32
#undef IMPL_GET_UINT16
#undef IMPL_GET_UINT32
#undef IMPL_GET_FLOAT
#undef IMPL_GET_STRING

#define IMPL_SET_BOOL(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(bool value) \
    { \
        _outer.ensureInit(); \
        _outer._doc[group][name] = value; \
    }
#define IMPL_SET_INT32(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(int32_t value) \
    { \
        _outer.ensureInit(); \
        if (value < (minv)) value = (minv); \
        if (value > (maxv)) value = (maxv); \
        _outer._doc[group][name] = value; \
    }
#define IMPL_SET_UINT16(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(uint16_t value) \
    { \
        _outer.ensureInit(); \
        if (value < static_cast<uint16_t>(minv)) value = static_cast<uint16_t>(minv); \
        if (value > static_cast<uint16_t>(maxv)) value = static_cast<uint16_t>(maxv); \
        _outer._doc[group][name] = value; \
    }
#define IMPL_SET_UINT32(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(uint32_t value) \
    { \
        _outer.ensureInit(); \
        if (value < static_cast<uint32_t>(minv)) value = static_cast<uint32_t>(minv); \
        if (value > static_cast<uint32_t>(maxv)) value = static_cast<uint32_t>(maxv); \
        _outer._doc[group][name] = value; \
    }
#define IMPL_SET_FLOAT(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(float value) \
    { \
        _outer.ensureInit(); \
        if (value < static_cast<float>(minv)) value = static_cast<float>(minv); \
        if (value > static_cast<float>(maxv)) value = static_cast<float>(maxv); \
        _outer._doc[group][name] = value; \
    }
#define IMPL_SET_STRING(group, name, api, def, minv, maxv) \
    void SettingsSetter::api(const String &value) \
    { \
        _outer.ensureInit(); \
        _outer._doc[group][name] = value; \
    }
#define SETTINGS_IMPL_SET(type, group, name, api, def, minv, maxv) IMPL_SET_##type(group, name, api, def, minv, maxv)
SETTINGS_ITEMS(SETTINGS_IMPL_SET)
#undef SETTINGS_IMPL_SET
#undef IMPL_SET_BOOL
#undef IMPL_SET_INT32
#undef IMPL_SET_UINT16
#undef IMPL_SET_UINT32
#undef IMPL_SET_FLOAT
#undef IMPL_SET_STRING
