#ifndef SOLAR2MQTT_SDKCONFIG_BT_DISABLE_H
#define SOLAR2MQTT_SDKCONFIG_BT_DISABLE_H

#include <sdkconfig.h>

// This project does not use Bluetooth. Neutralize the BT-related SDK config
// macros before any Arduino core source is parsed so optional BT helper units
// stay on their non-BT code paths.
#ifdef CONFIG_BT_ENABLED
#undef CONFIG_BT_ENABLED
#endif

#ifdef CONFIG_BT_CLASSIC_ENABLED
#undef CONFIG_BT_CLASSIC_ENABLED
#endif

#ifdef CONFIG_BT_BLE_ENABLED
#undef CONFIG_BT_BLE_ENABLED
#endif

#ifdef CONFIG_BLUEDROID_ENABLED
#undef CONFIG_BLUEDROID_ENABLED
#endif

#ifdef CONFIG_NIMBLE_ENABLED
#undef CONFIG_NIMBLE_ENABLED
#endif

#endif
