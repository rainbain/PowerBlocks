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

#include "wiimote.h"

#include "powerblocks/core/bluetooth/bltootls.h"

#include "wiimote_hid.h"
#include "wiimote_sys.h"

#include <string.h>

const char* WIIMOTE_TAG = "WIIMOTE";

wiimote_t WIIMOTES[WIIMOTE_MAX_REMOTES];

// Lookup table of what features are present in each reporting mode
static int wiimote_present_lookup_table[] = {
    // WIIMOTE_REPORT_BUTTONS
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_EXT8
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL_IR12
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_EXT19
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL_EXT16
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_IR10_EXT9
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // Unused
    0, 0, 0, 0, 0,

    // WIIMOTE_REPORT_EXT21
    0,

    // WIIMOTE_REPORT_INTERLEAVED_A
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_INTERLACED,

    // WIIMOTE_REPORT_INTERLEAVED_B
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_INTERLACED
};

static void wiimote_update_buttons(const wiimote_raw_t* raw_data, wiimote_t* output) {
    uint16_t new_state = (uint16_t)raw_data->data_report[0] | ((uint16_t)raw_data->data_report[1] << 8);
    uint16_t old_state = output->buttons.state;

    // Update state changes
    output->buttons.state = new_state;
    output->buttons.held = old_state;
    output->buttons.down = (~old_state) & new_state;
    output->buttons.up = old_state & (~old_state);
}

static void wiimote_update_accelerometer(const wiimote_raw_t* raw_data, wiimote_t* output) {
    int x, y, z;

    // Encoding of the data for these things is a bit weird.
    // In interlaced its all 8 bits of precision.
    // For all other modes, its 10 bit X, 9 bit Y and Z
    if(output->present & WIIMOTE_PRESENT_INTERLACED) {
        x = (int)raw_data->data_report[2] << 2;
        y = (int)raw_data->data_report[23] << 2;

        // Encoded in the unused button bits
        z = (int)(((raw_data->data_report[0] >> 5) & 0b11) << 4) << 2;
        z |= (int)(((raw_data->data_report[1] >> 5) & 0b11) << 6) << 2;
        z |= (int)(((raw_data->data_report[21] >> 5) & 0b11) << 0) << 2;
        z |= (int)(((raw_data->data_report[22] >> 5) & 0b11) << 2) << 2;
    } else {
        x = (int)raw_data->data_report[2] << 2;
        y = (int)raw_data->data_report[3] << 2;
        z = (int)raw_data->data_report[4] << 2;

        // LSB Data in the unused button bits
        x |= ((int)raw_data->data_report[0] >> 5) & 0b11;
        y |= (((int)raw_data->data_report[1] >> 5) & 0b1) << 1;
        z |= (((int)raw_data->data_report[1] >> 6) & 0b1) << 1;
    }

    // Make normalized floats
    const int zero = 0x200;
    output->accelerometer.x = (float)(x - zero) / 512.0f;
    output->accelerometer.y = (float)(y - zero) / 512.0f;
    output->accelerometer.z = (float)(z - zero) / 512.0f;
}

void wiimotes_initialize() {
    memset(WIIMOTES, 0, sizeof(WIIMOTES));

    bluetooth_driver_t driver = {
        .driver_id = BLUETOOTH_DRIVER_ID_WIIMOTE,
        .filter_device = wiimote_hid_driver_filter,
        .initialize_device = wiimote_hid_driver_initialize,
        .free_device = wiimote_hid_driver_free_device
    };

    bltools_register_driver(driver);

    // Load settings
    wiimote_sys_phrase_settings();

    // Connect wiimotes that have been connected in the past
    wiimote_connect_registered();
}

static void wiimote_update(const wiimote_raw_t* raw_data, wiimote_t* output) {
    // Bad Reporting Mode
    if(raw_data->report_type < 0x30 || raw_data->report_type > 0x3F)
        return;

    output->present = wiimote_present_lookup_table[raw_data->report_type - 0x30];

    if(output->present & WIIMOTE_PRESENT_BUTTONS) {
        wiimote_update_buttons(raw_data, output);
    }

    if(output->present & WIIMOTE_PRESENT_ACCELEROMETER) {
        wiimote_update_accelerometer(raw_data, output);
    }
}

void wiimote_poll() {
    wiimote_raw_t raw;
    for(int i = 0; i < WIIMOTE_MAX_REMOTES; i++) {
        if(WIIMOTES[i].driver == NULL)
            continue;
        
        wiimote_hid_t* hid = (wiimote_hid_t*)WIIMOTES[i].driver;

        // Quickly grab state
        xSemaphoreTake(hid->internal_state_lock, portMAX_DELAY);
        memcpy(&raw, &hid->internal_state, sizeof(raw));
        xSemaphoreGive(hid->internal_state_lock);


        wiimote_update(&raw, &WIIMOTES[i]);
    }
}

int wiimote_find_empty_slot() {
    for(int i = 0; i < WIIMOTE_MAX_REMOTES; i++) {
        if(WIIMOTES[i].driver == NULL) {
            return i;
        }
    }
    return -1;
}