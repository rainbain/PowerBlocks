#/**
 * @file ios_settings.h
 * @brief Settings from IOS
 *
 * Uses the system settings from IOS to get useful
 * system config.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Pull all the settings through IOS.
 * 
 * Pull all the settings through IOS.
 * Usually called by ios_initialize.
 */
extern void ios_settings_initialize();

/**
 * @brief Get a feild from the settings file.
 * 
 * Get a feild from the settings file.
 * Returns NULL if the key is not found.
 * 
 * You can see the keys here: https://wiibrew.org/wiki//title/00000001/00000002/data/setting.txt
 * @param key The key for the feild.
 * @return The value of the key in the setting file.
 */
extern const char* ios_settings_get(const char* key);

/**
 * @brief Get a feild from the config file.
 * 
 * Get a feild from the config file.
 * Returns the length of the value retrieved or zero if it could not be found.
 * 
 * You can see the keys and format here: https://wiibrew.org/wiki//shared2/sys/SYSCONF
 * @param key The key for the feild.
 * @param buffer Buffer to put the data into
 * @param size Size of the buffer
 * @return The value of the key in the setting file.
 */
extern uint32_t ios_config_get(const char* key, void* buffer, uint32_t size);

/**
 * @brief Returns true if the default video mode is progressive scan.
 * 
 * Returns true if the default video mode is progressive scan.
 * @return >0 = Progressive Scan else Interlaced
 */
extern bool ios_config_is_progressive_scan();

/**
 * @brief Get a feild from the config file.
 * 
 * Returns true if the default video mode is eurgb60
 * @return >0 = eurgb60 else 50hz
 */
extern bool ios_config_is_eurgb60();