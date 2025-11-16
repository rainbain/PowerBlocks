/**
 * @file wiimote_extension.c
 * @brief Handles wiimote extensions.
 *
 * Decodes the data of the various wiimote extensions for the user to use.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "wiimote_log.h"

#include "wiimote_extension.h"

#include <string.h>

static int phrase_nunchuck(wiimote_extension_data_t* out, const uint8_t* data, size_t len) {
    if(len < 6)
        return -1;
    
    int stick_x = (int)data[0] - 128;
    int stick_y = (int)data[1] - 128;
    int accel_x = (int)data[2] << 2;
    int accel_y = (int)data[3] << 2;
    int accel_z = (int)data[4] << 2;
    accel_x |= (int)(data[5] >> 6) & 0b11;
    accel_y |= (int)(data[5] >> 4) & 0b11;
    accel_z |= (int)(data[5] >> 2) & 0b11;
    uint16_t next_button_state = data[5] & 0b11;

    // Inverted
    next_button_state ^= 0b11;

    out->type = WIIMOTE_EXTENSION_NUNCHUK;
    wiimote_set_button_helper(&out->nunchuck.buttons, next_button_state);

    // The stick is weirdly unbalanced. Taken from the wiibrew page
    out->nunchuck.stick.x = (float)stick_x / ((stick_x > 0) ? 100.0f : 93.0f);
    out->nunchuck.stick.y = (float)stick_y / ((stick_y > 0) ? 92.0f : 101.0f);

    // Accelerometer data works like it does for wiimote.
    out->nunchuck.accelerometer.x = (float)(accel_x - 512) / 512.0f;
    out->nunchuck.accelerometer.y = (float)(accel_y - 512) / 512.0f;
    out->nunchuck.accelerometer.z = (float)(accel_z - 512) / 512.0f;
    return 0;
}

static int phrase_classic_controller(wiimote_extension_data_t* out, const uint8_t* data, size_t len) {
    if(len < 6)
        return -1;

    // For now we will assume data format 0x01,
    // Its not the most precise mode, but it will do.

    int left_stick_x = (int)data[0] & 0b111111;
    int left_stick_y = (int)data[1] & 0b111111;
    int right_stick_x = ((int)data[2] >> 7) | ((((int)data[1] >> 6) & 0b11) << 1) | ((((int)data[0] >> 6) & 0b11) << 3);
    int right_stick_y = (int)data[2] & 0b11111;
    int left_trigger = ((int)data[3] >> 5) | (((((int)data[2] >> 5)) & 0b11) << 3);
    int right_trigger = (int)data[3] & 0b11111;

    uint16_t next_button_state = ((uint32_t)data[4] << 8) | (uint32_t)data[5];

    // Inverted
    next_button_state ^= 0xFFFF;

    out->type = WIIMOTE_EXTENSION_CLASSIC_CONTROLLER;
    wiimote_set_button_helper(&out->classic_controller.buttons, next_button_state);
    out->classic_controller.left_stick.x = (float)left_stick_x / 63.0f * 2.0f - 1.0f;
    out->classic_controller.left_stick.y = (float)left_stick_y / 63.0f * 2.0f - 1.0f;
    out->classic_controller.right_stick.x = (float)right_stick_x / 31.0f * 2.0f - 1.0f;
    out->classic_controller.right_stick.y = (float)right_stick_y / 31.0f * 2.0f - 1.0f;

    out->classic_controller.triggers.x = (float)left_trigger / 31.0f;
    out->classic_controller.triggers.y = (float)right_trigger / 31.0f;
    return 0;
}

static const wiimote_extension_mapper_t nunchuck_mapper = {
    .type = WIIMOTE_EXTENSION_NUNCHUK,
    .phrase_data = phrase_nunchuck
};

static const wiimote_extension_mapper_t classic_controller_mapper = {
    .type = WIIMOTE_EXTENSION_CLASSIC_CONTROLLER,
    .phrase_data = phrase_classic_controller
};

void wiimote_extension_phrase_data(wiimote_extension_data_t* out, const wiimote_extension_mapper_t* mapper, const uint8_t* data, size_t size) {
    // No mapper, clear it out
    if(mapper == NULL) {
        memset(out, 0, sizeof(*out));
        return;
    }

    int ret;
    ret = mapper->phrase_data(out, data, size);

    // Invalid data? Clear it out
    if(ret < 0) {
        memset(out, 0, sizeof(*out));
    }
}

wiimote_extension_t wiimote_extension_get_type(const uint8_t* byte_code) {
    // Turn the byte code into 64 bit number for easy comparison
    uint64_t type = ((uint64_t)byte_code[0] << (5 * 8)) |  ((uint64_t)byte_code[1] << (4 * 8)) |  ((uint64_t)byte_code[2] << (3 * 8)) |
                    ((uint64_t)byte_code[3] << (2 * 8)) |  ((uint64_t)byte_code[4] << (1 * 8)) |  ((uint64_t)byte_code[5] << (0 * 8));
    
    switch(type) {
        case 0x0000A4200000:
            return WIIMOTE_EXTENSION_NUNCHUK;
        case 0x0000A4200101:
            return WIIMOTE_EXTENSION_CLASSIC_CONTROLLER;
        case 0x0100A4200101:
            return WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PRO;
        case 0xFF00A4200013:
            return WIIMOTE_EXTENSION_DRAWSOME_GRAPHICS_TABLET;
        case 0x0000A4200103:
            return WIIMOTE_EXTENSION_GH_GUITAR;
        case 0x0100A4200103:
            return WIIMOTE_EXTENSION_GH_DRUMS;
        case 0x0300A4200103:
            return WIIMOTE_EXTENSION_DJ_HERO_TURNTABLE;
        case 0x0000A4200111:
            return WIIMOTE_EXTENSION_TAIKO_DRUMS;
        case 0xFF00A4200112:
            return WIIMOTE_EXTENSION_UDRAW_GAME_TABLET;
        case 0x0000A4200310:
            return WIIMOTE_EXTENSION_SHINKANSEN_CONTROLLER;
        case 0x0000A4200402:
            return WIIMOTE_EXTENSION_BALANCE_BOARD;
        default:
            WIIMOTE_LOG_ERROR("Unknown extension code: %012X", type);
            return WIIMOTE_EXTENSION_NONE;
    }
}

const char* wiimote_extension_get_name(wiimote_extension_t type) {
    switch(type) {
        case WIIMOTE_EXTENSION_NUNCHUK:
            return "Nunchuk";
        case WIIMOTE_EXTENSION_CLASSIC_CONTROLLER:
            return "Classic Controller";
        case WIIMOTE_EXTENSION_CLASSIC_CONTROLLER_PRO:
            return "Classic Controller Pro";
        case WIIMOTE_EXTENSION_DRAWSOME_GRAPHICS_TABLET:
            return "Drawsome Graphics Tablet";
        case WIIMOTE_EXTENSION_GH_GUITAR:
            return "Guitar Hero Guitar";
        case WIIMOTE_EXTENSION_GH_DRUMS:
            return "Guitar Hero Drums";
        case WIIMOTE_EXTENSION_DJ_HERO_TURNTABLE:
            return "DJ Hero Turntable";
        case WIIMOTE_EXTENSION_TAIKO_DRUMS:
            return "Taiko no Tatsujin TaTaCon Drum controller";
        case WIIMOTE_EXTENSION_UDRAW_GAME_TABLET:
            return "UDraw Game Tablet";
        case WIIMOTE_EXTENSION_SHINKANSEN_CONTROLLER:
            return "Densha de GO! Shinkansen Controller";
        case WIIMOTE_EXTENSION_BALANCE_BOARD:
            return "Wii Balance Board";
        default:
            return "Unknown";
    }
}

const wiimote_extension_mapper_t* wiimote_extension_get_mapper(wiimote_extension_t type) {
    switch(type) {
        case WIIMOTE_EXTENSION_NUNCHUK:
            return &nunchuck_mapper;
        case WIIMOTE_EXTENSION_CLASSIC_CONTROLLER:
            return &classic_controller_mapper;
        default:
            return NULL;
    }
}

void wiimote_set_button_helper(wiimote_buttons* buttons, uint16_t next_state) {
    uint16_t old_state = buttons->state;

    buttons->state = next_state;
    buttons->held = old_state;
    buttons->down = (~old_state) & next_state;
    buttons->up = old_state & (~next_state);
}