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
#include "powerblocks/core/utils/math/vec2.h"

#include <stdbool.h>
#include <stdint.h>

// Flags of the present byte that show what
// WiiMote data is present in the state.
// Non present data will just hold its previous state.
#define WIIMOTE_PRESENT_BUTTONS       (1<<0)
#define WIIMOTE_PRESENT_ACCELEROMETER (1<<1)
#define WIIMOTE_PRESENT_IR            (1<<2)
#define WIIMOTE_PRESENT_IR_EXTENDED   (1<<3)
#define WIIMOTE_PRESENT_IR_FULL       (1<<4)
#define WIIMOTE_PRESENT_INTERLACED    (1<<5) // This signals the data was collected using interlaced input packets

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

    // Raw Accelerometer Data. Updated if accelerometer is enabled.
    vec3 accelerometer;

    // Raw IR Data
    // Up to 4 IR points tracked. If more than 4 it will switch between them.
    struct {
        // Included in all IR modes, 10 bit unsigned
        bool visible;
        vec2i position;

        // Included in extended and full mode. Size 0-15
        int size;

        // Bounding Box, and intensity of point. Included only in full mode.
        vec2i bbox_top_left;
        vec2i bbox_bottom_right;
        uint8_t intensity;
    } ir_tracking[4];

    // Cursor position, calculated from IR tracking if enabled.
    // Pixel position on screen. Can extend off screen.
    vec2i cursor;
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

// Sets what data the wiimote will report
// Same thing as the "present" stuff
extern int wiimote_set_reporting(wiimote_t* wiimote, int present);