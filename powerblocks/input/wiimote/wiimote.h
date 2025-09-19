/**
 * @file wiimote_parsing.h
 * @brief Bluetooth Instance / Protocol Of The WiiMote
 *
 * Contains the code to phrase data from wiimotes into something useful.
 * 
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/utils/math/vec3.h"

#include <stdbool.h>
#include <stdint.h>

// Flags of the present byte that show what
// WiiMote data is present in the state.
// Non present data will just hold its previous state.
#define WIIMOTE_PRESENT_BUTTONS       (1<<0)
#define WIIMOTE_PRESENT_ACCELEROMETER (1<<1)
#define WIIMOTE_PRESENT_INTERLACED    (1<<2) // This signals the data was collected using interlaced input packets

#define WIIMOTE_BUTTONS_DPAD_LEFT  (1<<0)
#define WIIMOTE_BUTTONS_DPAD_RIGHT (1<<1)
#define WIIMOTE_BUTTONS_DPAD_DOWN  (1<<2)
#define WIIMOTE_BUTTONS_DPAD_UP    (1<<3)
#define WIIMOTE_BUTTONS_PLUS       (1<<4)
#define WIIMOTE_BUTTONS_TWO        (1<<8)
#define WIIMOTE_BUTTONS_ONE        (1<<9)
#define WIIMOTE_BUTTONS_B          (1<<10)
#define WIIMOTE_BUTTONS_A          (1<<11)
#define WIIMOTE_BUTTONS_MINUS      (1<<12)
#define WIIMOTE_BUTTONS_HOME       (1<<15)

// Max remotes that can be connected.
#define WIIMOTE_MAX_REMOTES 4

typedef struct {
    // True if this wiimote has been connected.
    void *driver;

    // Holds what kind of data is present.
    uint32_t present;

    // Include the "Core Buttons" of the wiimote. The ones on the remote itself
    struct {
        uint16_t state; // Current frame of data. What ones are true and false
        uint16_t held; // Last frame of data. "Held" buttons
        uint16_t down; // True for 1 poll when pushed down
        uint16_t up; // True for 1 poll when up
    } buttons;

    // Raw Accelerometer Data
    vec3 accelerometer;
} wiimote_t;

extern wiimote_t WIIMOTES[WIIMOTE_MAX_REMOTES];

// Initializes motes and registers drivers.
// Bluetooth must be initialized
// IOS Settings must be initialized
extern void wiimotes_initialize();

extern void wiimote_poll();

// Get the next empty slot
// -1 if not slot
extern int wiimote_find_empty_slot();
