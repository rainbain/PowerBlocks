/**
 * @file wiimote_extension.h
 * @brief Handles wiimote extensions.
 *
 * Decodes the data of the various wiimote extensions for the user to use.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "powerblocks/core/utils/math/vec3.h"
#include "powerblocks/core/utils/math/vec2.h"

typedef enum {
    WIIMOTE_EXTENSION_NONE = 0,
    WIIMOTE_EXTENSION_NUNCHUK,
    WIIMOTE_EXTENSION_CLASSIC_CONTROLLER,
    WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PRO,
    WIIMOTE_EXTENSION_DRAWSOME_GRAPHICS_TABLET,
    WIIMOTE_EXTENSION_GH_GUITAR,
    WIIMOTE_EXTENSION_GH_DRUMS,
    WIIMOTE_EXTENSION_DJ_HERO_TURNTABLE,
    WIIMOTE_EXTENSION_TAIKO_DRUMS,
    WIIMOTE_EXTENSION_UDRAW_GAME_TABLET,
    WIIMOTE_EXTENSION_SHINKANSEN_CONTROLLER,
    WIIMOTE_EXTENSION_BALANCE_BOARD
} wiimote_extension_t;

#define WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_Z (1<<0)
#define WIIMOTE_EXTENSION_NUNCHUCK_BUTTONS_C (1<<1)

// Works for classic controller pro too.
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_UP       (1<<0)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_LEFT     (1<<1)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_RIGHT       (1<<2)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_X             (1<<3)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_A             (1<<4)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Y             (1<<5)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_B             (1<<6)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_Z_LEFT        (1<<7)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_RIGHT_TRIGGER (1<<9)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PLUS          (1<<10)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_HOME          (1<<11)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_MINUS         (1<<12)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_LEFT_TRIGGER  (1<<13)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_DOWN     (1<<14)
#define WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_DPAD_RIGHT    (1<<15)

typedef struct {
        uint16_t state; // Current frame of data. What ones are true and false
        uint16_t held; // Last frame of data. "Held" buttons
        uint16_t down; // True for 1 poll when pushed down
        uint16_t up; // True for 1 poll when up
} wiimote_buttons;

typedef struct {
    wiimote_extension_t type;
    union {
        struct {
            wiimote_buttons buttons;
            vec2 stick;
            vec3 accelerometer;
        } nunchuck;

        struct {
            wiimote_buttons buttons;
            vec2 left_stick;
            vec2 right_stick;
            vec2 triggers; // {left, right}
        } classic_controller;
    };
} wiimote_extension_data_t;

typedef int (*wiimote_extension_handle_phrase)(wiimote_extension_data_t* out, const uint8_t* data, size_t len);

typedef struct {
    wiimote_extension_t type;
    wiimote_extension_handle_phrase phrase_data;
} wiimote_extension_mapper_t;

// Attemps to phrase the wiimote extension data.
// NULL is acceptable for a mapper
extern void wiimote_extension_phrase_data(wiimote_extension_data_t* out, const wiimote_extension_mapper_t* mapper, const uint8_t* data, size_t size);

// Returns the wiimote extension type enum
// based on the 6 byte code from 0x04A40000 in the wiimote
extern wiimote_extension_t wiimote_extension_get_type(const uint8_t* byte_code);

// Prints an error if an extension type is unsupported
extern const char* wiimote_extension_get_name(wiimote_extension_t type);

// Gets the extension mapper for a controller, or NULL if unknown
extern const wiimote_extension_mapper_t* wiimote_extension_get_mapper(wiimote_extension_t type);

// Generic helper for settings buttons
extern void wiimote_set_button_helper(wiimote_buttons* buttons, uint16_t next_state);