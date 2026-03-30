#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "core/SettingsPrefs.schema.h"

class Settings;

class SettingsGetter
{
public:
    explicit SettingsGetter(Settings &outer);

#define DECL_GET_BOOL(group, name, api, def, minv, maxv) bool api();
#define DECL_GET_INT32(group, name, api, def, minv, maxv) int32_t api();
#define DECL_GET_UINT16(group, name, api, def, minv, maxv) uint16_t api();
#define DECL_GET_UINT32(group, name, api, def, minv, maxv) uint32_t api();
#define DECL_GET_FLOAT(group, name, api, def, minv, maxv) float api();
#define DECL_GET_STRING(group, name, api, def, minv, maxv) const char *api();
#define SETTINGS_DECL_GET(type, group, name, api, def, minv, maxv) DECL_GET_##type(group, name, api, def, minv, maxv)
    SETTINGS_ITEMS(SETTINGS_DECL_GET)
#undef SETTINGS_DECL_GET
#undef DECL_GET_BOOL
#undef DECL_GET_INT32
#undef DECL_GET_UINT16
#undef DECL_GET_UINT32
#undef DECL_GET_FLOAT
#undef DECL_GET_STRING

private:
    Settings &_outer;
};

class SettingsSetter
{
public:
    explicit SettingsSetter(Settings &outer);

#define DECL_SET_BOOL(group, name, api, def, minv, maxv) void api(bool value);
#define DECL_SET_INT32(group, name, api, def, minv, maxv) void api(int32_t value);
#define DECL_SET_UINT16(group, name, api, def, minv, maxv) void api(uint16_t value);
#define DECL_SET_UINT32(group, name, api, def, minv, maxv) void api(uint32_t value);
#define DECL_SET_FLOAT(group, name, api, def, minv, maxv) void api(float value);
#define DECL_SET_STRING(group, name, api, def, minv, maxv) void api(const String &value);
#define SETTINGS_DECL_SET(type, group, name, api, def, minv, maxv) DECL_SET_##type(group, name, api, def, minv, maxv)
    SETTINGS_ITEMS(SETTINGS_DECL_SET)
#undef SETTINGS_DECL_SET
#undef DECL_SET_BOOL
#undef DECL_SET_INT32
#undef DECL_SET_UINT16
#undef DECL_SET_UINT32
#undef DECL_SET_FLOAT
#undef DECL_SET_STRING

private:
    Settings &_outer;
};

class Settings
{
public:
    Settings();

    void begin();
    bool save();
    String backup(bool pretty = false);
    bool restore(const String &json, bool saveAfter = true);

    SettingsGetter get;
    SettingsSetter set;

private:
    friend class SettingsGetter;
    friend class SettingsSetter;

    void ensureInit();
    void loadFromNvs();
    void writeToNvs();

    bool _initialized;
    JsonDocument _doc;
};
