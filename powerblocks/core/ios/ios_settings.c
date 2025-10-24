#/**
 * @file ios_settings.c
 * @brief Settings from IOS
 *
 * Uses the system settings from IOS to get useful
 * system config.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "ios_settings.h"

#include <string.h>

#include "system.h"
#include "ios.h"

#include "utils/log.h"

static const char* TAB = "IOS_SETTINGS";

// A staticly sized buffer to hold data from the settings file.
#define IOS_SETTINGS_LENGTH 256

// This is of a fixed size determined by IOS
#define IOS_CONFIG_LENGTH 0x4000

// File formats documented on wiibrew.
static const char* ios_settings_file_path ALIGN(32) = "/title/00000001/00000002/data/setting.txt";
static const char* ios_config_file_path ALIGN(32) = "/shared2/sys/SYSCONF";

static char ios_settings_data[IOS_SETTINGS_LENGTH + 1] ALIGN(32);
static uint8_t ios_config_data[IOS_CONFIG_LENGTH] ALIGN(32);

static int settings_field_count;

void ios_settings_initialize() {
    // Open file and read data.
    int fd = ios_open(ios_settings_file_path, IOS_MODE_READ);
    ASSERT(fd >= 0);

    memset(ios_settings_data, 0, sizeof(ios_settings_data)); // Adds zero termination too
    int ret = ios_read(fd, ios_settings_data, IOS_SETTINGS_LENGTH);
    ASSERT(ret >= 0);

    ret = ios_close(fd);
    ASSERT(ret >= 0);

    // Decrypt the file
    uint32_t key = 0x73B5DBFA;
    for(int i = 0; i < IOS_SETTINGS_LENGTH; i++) {
        ios_settings_data[i] = ios_settings_data[i] ^ key;
        key = (key << 1) | (key >> 31);
    }

    // Discover each field
    settings_field_count = 0;
    char* str = ios_settings_data;
    while(*str) {
        if(*str == '\n') {
            *str = 0;
            settings_field_count++;
        }

        if(*str == '\r') {
            *str = 0;
        }

        str++;
    }

    // Next do the config file for the system
    fd = ios_open(ios_config_file_path, IOS_MODE_READ);
    ASSERT(fd >= 0);

    memset(ios_config_data, 0, sizeof(ios_config_data)); // Adds zero termination too
    ret = ios_read(fd, ios_config_data, sizeof(ios_config_data));
    ASSERT(ret >= 0);

    ret = ios_close(fd);
    ASSERT(ret >= 0);

    LOG_INFO(TAB, "IOS Settings Initialize.");
}

const char* ios_settings_get(const char* key) {
    const char* p = ios_settings_data;
    int count = 0;
    size_t key_len = strlen(key);

    while (count < settings_field_count) {
        if (*p == '\0') {
            // Skip over extra nulls
            p++;
            continue;
        }

         // Ffind '=' separator
        const char* eq = strchr(p, '=');
        if (!eq) {
            // Malformed entry so skip to next
            while (*p != '\0') p++;
            continue;
        }

        size_t cur_key_len = eq - p;

        // Compare keys
        if (cur_key_len == key_len && strncmp(p, key, key_len) == 0) {
            // Return pointer to value
            return eq + 1;
        }

        // Skip to next entry
        while (*p != '\0') p++;
        count++;
    }

    return NULL;
}

uint32_t ios_config_get(const char* key, void* buffer, uint32_t size) {
    // Find the entry by looking for its name
    int entry_count = *(const uint16_t*)(ios_config_data + 4);
    const uint16_t* offsets = (const uint16_t*)(ios_config_data + 6);

    int key_length = strlen(key);

    int found_entry_type;
    const uint8_t* found_entry_value = NULL;

    for(int i = 0; i < entry_count; i++) {
        const uint8_t* entry = ios_config_data + offsets[i];

        int entry_key_length = (entry[0] & 0x1F) + 1;
        if(entry_key_length != key_length)
            continue;
        
        if(memcmp(key, entry + 1, key_length) != 0)
            continue;
        
        found_entry_type = entry[0] >> 5;
        found_entry_value = entry + 1 + key_length;
    }

    // Entry not Found
    if(found_entry_value == NULL) {
        return 0;
    }

    // Decode the length of the entry
    int entry_length;
    switch(found_entry_type) {
        case 1: // BIGARRAY
            entry_length = *((const uint16_t*)found_entry_value) + 1;
            found_entry_value += 2;
            break;
        case 2: // SMALLARRAY
            entry_length = (*found_entry_value) + 1;
            found_entry_value += 1;
            break;
        case 3: // BYTE
            entry_length = 1;
            break;
        case 4: // SHORT
            entry_length = 2;
            break;
        case 5: // LONG
            entry_length = 4;
            break;
        case 6: // LONGLONG
            entry_length = 8;
            break;
        case 7: // BOOL
            entry_length = 1;
            break;
        default:
            LOG_ERROR(TAB, "Unknown key type: %X\n", found_entry_type);
            return 0;
    }

    // Copy data
    int copy_size = entry_length > size ? size : entry_length;
    memcpy(buffer, found_entry_value, copy_size);

    return entry_length;
}

static void ios_config_get_with_check(const char* key, void* buffer, uint32_t size) {
    int ret = ios_config_get(key, buffer, size);
    if(size != ret) {
        if(ret == 0) {
            LOG_ERROR(TAB, "Could not find key: \"%s\"\n", key);
        } else {
            LOG_ERROR(TAB, "Key size mismatch, expected %d, got %d\n", size, ret);
        }
        memset(buffer, 0, size);
    }
}

bool ios_config_is_progressive_scan() {
    bool val;
    ios_config_get_with_check("IPL.PGS", &val, sizeof(val));

    return val;
}

bool ios_config_is_eurgb60() {
    bool val;
    ios_config_get_with_check("IPL.E60", &val, sizeof(val));

    return val;
}