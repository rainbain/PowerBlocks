/**
 * @file ios_settings.c
 * @brief Settings from IOS
 *
 * Handles system wide wiimote settings.
 * These are the wiimote settings specified in 
 * /shared2/sys/SYSCONF
 * 
 * It contains like, if the motor is enabled, and connection addresses.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "wiimote_sys.h"

#include "powerblocks/core/ios/ios_settings.h"

#include "powerblocks/core/bluetooth/bltootls.h"

#include <string.h>

#include "wiimote_log.h"

wiimote_sys_config_t wiimote_sys_config;

void wiimote_sys_phrase_settings() {
    // Clear to zeros to be safe
    memset(&wiimote_sys_config, 0, sizeof(wiimote_sys_config));

    const char* setting;
    uint32_t length;

    setting = "BT.BAR";
    length = ios_config_get(setting, &wiimote_sys_config.sensor_bar_position, sizeof(wiimote_sys_config.sensor_bar_position));
    if(length != sizeof(wiimote_sys_config.sensor_bar_position))
        goto ERROR;
    
    setting = "BT.CDIF";
    length = ios_config_get(setting, &wiimote_sys_config.guest_wiimotes, sizeof(wiimote_sys_config.guest_wiimotes));
    if(length != sizeof(wiimote_sys_config.guest_wiimotes))
        goto ERROR;
    
    setting = "BT.DINF";
    length = ios_config_get(setting, &wiimote_sys_config.wiimotes, sizeof(wiimote_sys_config.wiimotes));
    if(length != sizeof(wiimote_sys_config.wiimotes))
        goto ERROR;
    
    setting = "BT.MOT";
    length = ios_config_get(setting, &wiimote_sys_config.motor_enabled, sizeof(wiimote_sys_config.motor_enabled));
    if(length != sizeof(wiimote_sys_config.motor_enabled))
        goto ERROR;
    
    setting = "BT.SENS";
    length = ios_config_get(setting, &wiimote_sys_config.ir_sensitivity, sizeof(wiimote_sys_config.ir_sensitivity));
    if(length != sizeof(wiimote_sys_config.ir_sensitivity))
        goto ERROR;
    
    setting = "BT.SPKV";
    length = ios_config_get(setting, &wiimote_sys_config.speaker_volume, sizeof(wiimote_sys_config.speaker_volume));
    if(length != sizeof(wiimote_sys_config.speaker_volume))
        goto ERROR;
    
    if(wiimote_sys_config.guest_wiimotes.count > 6) {
        WIIMOTE_LOG_ERROR("More guest wiimotes than possible.");
        goto ERROR;
    }

    if(wiimote_sys_config.wiimotes.count > 6) {
        WIIMOTE_LOG_ERROR("More guest wiimotes than possible.");
        goto ERROR;
    }

    WIIMOTE_LOG_DEBUG("WiiMote Settings:");
    WIIMOTE_LOG_DEBUG("  Sensor Bar:     %s", wiimote_sys_config.sensor_bar_position ? "TOP" : "BOTTOM");
    WIIMOTE_LOG_DEBUG("  Motor Enabled:  %s", wiimote_sys_config.motor_enabled ? "True" : "False");
    WIIMOTE_LOG_DEBUG("  IR Sensitivity: %d", wiimote_sys_config.ir_sensitivity);
    WIIMOTE_LOG_DEBUG("  Speaker Volume: %d", wiimote_sys_config.speaker_volume);
    
    WIIMOTE_LOG_DEBUG("  Guest Wiimotes (%d):", wiimote_sys_config.guest_wiimotes.count);
    for(int i = 0; i < wiimote_sys_config.guest_wiimotes.count; i++) {

        WIIMOTE_LOG_DEBUG("    %02X:%02X:%02X:%02X:%02X:%02X %s",
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[5],
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[4],
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[3],
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[2],
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[1],
        wiimote_sys_config.guest_wiimotes.entrys[i].mac_address[0],
        wiimote_sys_config.guest_wiimotes.entrys[i].name);
    }
    WIIMOTE_LOG_DEBUG("  Registered Wiimotes (%d):", wiimote_sys_config.wiimotes.count);
    for(int i = 0; i < wiimote_sys_config.wiimotes.count; i++) {
        WIIMOTE_LOG_DEBUG("    %02X:%02X:%02X:%02X:%02X:%02X %s",
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[5],
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[4],
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[3],
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[2],
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[1],
        wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[0],
        wiimote_sys_config.wiimotes.registered_entrys[i].name);
    }
    return;
ERROR:
    // I would like to avoid exploding if we get a bad setting
    WIIMOTE_LOG_ERROR("Failed to phrase config setting \"%s\", length %d. Using defaults.", setting, length);
    memset(&wiimote_sys_config, 0, sizeof(wiimote_sys_config));
}

void wiimote_connect_registered() {
    hci_discovered_device_info_t info;
    info.page_scan_repetition_mode = 0x01; // How often to scan, default "safe" value
    info.class_of_device = 0; /// TODO: Maybe put wiimote COD? Eh, it will find it by name
    info.clock_offset = 0; // Unknow - HCI find one for us

    for(int i = 0; i < wiimote_sys_config.wiimotes.count; i++) {
        for(int x = 0; x < 6; x++) {
            info.address[x] = wiimote_sys_config.wiimotes.registered_entrys[i].mac_address[5-x];
        }
        bltools_load_compatable_driver(&info, wiimote_sys_config.wiimotes.registered_entrys[i].name);
    }
}