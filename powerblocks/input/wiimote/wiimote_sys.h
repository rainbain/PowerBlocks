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

#pragma once

#include "powerblocks/core/system/system.h"

#include <stdint.h>

// Data structure describing the system config of wiimotes.
// Changes to these WILL NOT be written back and reflected on the NAND as of currently for safety.
// Documentation of these settings on https://wiibrew.org/wiki//shared2/sys/SYSCONF.
typedef struct {
    uint8_t sensor_bar_position; // Sensor bar on bottom = 0, or top = 1, of wii.
    uint8_t motor_enabled; // Is the Motors globally enabled?
    uint32_t ir_sensitivity; // IR Sensor sensitivity setting
    uint8_t speaker_volume; // Volume of the speaker in the wiimote

    // Stores wiimotes connected to the wii while running via the "1" "2" button combo.
    struct {
        uint8_t count; // Number of guest wiimotes.
        struct {
            uint8_t mac_address[6]; // Stored backwards, wii cool like that
            char name[64];
            uint8_t link_key[16];
        } entrys[6] PACKED;
    } guest_wiimotes PACKED;



    // Stores registered and active wiimotes.
    struct {
        uint8_t count; // Number of registered wiimotes.
        struct {
            uint8_t mac_address[6]; // Stored backwards, wii cool like that
            char name[64];
        } registered_entrys[10];

        // Runtime active wiimote
        struct {
            uint8_t mac_address[6]; // Stored backwards, wii cool like that
            char name[64];
        } active_wiimotes[6];
    } wiimotes PACKED;
} wiimote_sys_config_t;

extern wiimote_sys_config_t wiimote_sys_config;

// Phrases the wiimote settings from SYSCONFIG
// If this fails, config will load to defaults
extern void wiimote_sys_phrase_settings();

// Attempt adding registered wiimotes
extern void wiimote_connect_registered();