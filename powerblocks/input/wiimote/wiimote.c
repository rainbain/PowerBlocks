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

#include "powerblocks/core/system/gpio.h"
#include "powerblocks/core/bluetooth/bltootls.h"
#include "powerblocks/core/bluetooth/blerror.h"

#include "wiimote_hid.h"
#include "wiimote_sys.h"

#include <string.h>

const char* WIIMOTE_TAG = "WIIMOTE";

wiimote_t WIIMOTES[WIIMOTE_MAX_REMOTES];

// Lookup table of what features are present in each reporting mode
static const int wiimote_present_lookup_table[] = {
    // WIIMOTE_REPORT_BUTTONS
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_EXT8
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL_IR12
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_IR_EXTENDED,

    // WIIMOTE_REPORT_BUTTONS_EXT19
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL_EXT16
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_IR10_EXT9
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_IR,

    // WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_IR,

    // Unused
    0, 0, 0, 0, 0,

    // WIIMOTE_REPORT_EXT21
    0,

    // WIIMOTE_REPORT_INTERLEAVED_A
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_INTERLACED | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_IR_FULL,

    // WIIMOTE_REPORT_INTERLEAVED_B
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_INTERLACED | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_IR_FULL
};

// Lookup table for the order to prefer modes from the previous table, higher modes selected first
static const uint8_t wiimote_present_preferred_table[] =  {
    0, 1, 2, 3, 4, 5, 6, 3, 14
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

static void wiimote_update_ir(const wiimote_raw_t* raw_data, wiimote_t* output) {
    int mode = output->present & (WIIMOTE_PRESENT_IR_EXTENDED | WIIMOTE_PRESENT_IR_FULL);

    // If accelerometer is present, IR data starts 5 bytes in and not 2.
    int start = (output->present & WIIMOTE_PRESENT_ACCELEROMETER) ? 5 : 2;

    if(mode == 0) { // Basic Mode
        // Pairs of 2 in 5 bytes
        for(int i = 0; i < 2; i ++) {
            output->ir_tracking[i*2+0].position.x = raw_data->data_report[start + 0 + i*5];
            output->ir_tracking[i*2+0].position.y = raw_data->data_report[start + 1 + i*5];
            output->ir_tracking[i*2+1].position.x = raw_data->data_report[start + 3 + i*5];
            output->ir_tracking[i*2+1].position.y = raw_data->data_report[start + 4 + i*5];

            // Last bits in this middle byte
            uint8_t t = raw_data->data_report[start + 2 + i*5];
            output->ir_tracking[i*2+0].position.y |= ((t >> 6) & 0b11) << 8;
            output->ir_tracking[i*2+0].position.x |= ((t >> 4) & 0b11) << 8;
            output->ir_tracking[i*2+1].position.y |= ((t >> 2) & 0b11) << 8;
            output->ir_tracking[i*2+1].position.x |= ((t >> 0) & 0b11) << 8;
        }
    } else if(mode == WIIMOTE_PRESENT_IR_EXTENDED) { // Extended Mode
        for(int i = 0; i < 4; i++) {
            output->ir_tracking[i].position.x = raw_data->data_report[start + 0 + i*3];
            output->ir_tracking[i].position.y = raw_data->data_report[start + 1 + i*3];

            // Last bits in last byte
            uint8_t t = raw_data->data_report[start + 1 + i*3];
            output->ir_tracking[i].position.y |= ((t >> 6) & 0b11) << 8;
            output->ir_tracking[i].position.x |= ((t >> 4) & 0b11) << 8;
            output->ir_tracking[i].size = t & 0b1111;
        }
    } else { // Full Mode
        for(int i = 0; i < 2; i++) {
            // First report
            output->ir_tracking[0+i].position.x = raw_data->data_report[3+i*9 + 0];
            output->ir_tracking[0+i].position.y = raw_data->data_report[3+i*9 + 1];
            output->ir_tracking[0+i].bbox_top_left.x = raw_data->data_report[3+i*9 + 3];
            output->ir_tracking[0+i].bbox_top_left.y = raw_data->data_report[3+i*9 + 4];
            output->ir_tracking[0+i].bbox_bottom_right.x = raw_data->data_report[3+i*9 + 5];
            output->ir_tracking[0+i].bbox_bottom_right.y = raw_data->data_report[3+i*9 + 6];
            output->ir_tracking[0+i].intensity = raw_data->data_report[3+i*9 + 8];

            uint8_t t = raw_data->data_report[3+i*9 + 2];
            output->ir_tracking[0+i].position.y |= ((t >> 6) & 0b11) << 8;
            output->ir_tracking[0+i].position.x |= ((t >> 4) & 0b11) << 8;
            output->ir_tracking[0+i].size = t & 0b1111;


            // Second report
            output->ir_tracking[2+i].position.x = raw_data->data_report[24+i*9 + 0];
            output->ir_tracking[2+i].position.y = raw_data->data_report[24+i*9 + 1];
            output->ir_tracking[2+i].bbox_top_left.x = raw_data->data_report[24+i*9 + 3];
            output->ir_tracking[2+i].bbox_top_left.y = raw_data->data_report[24+i*9 + 4];
            output->ir_tracking[2+i].bbox_bottom_right.x = raw_data->data_report[24+i*9 + 5];
            output->ir_tracking[2+i].bbox_bottom_right.y = raw_data->data_report[24+i*9 + 6];
            output->ir_tracking[2+i].intensity = raw_data->data_report[24+i*9 + 8];

            t = raw_data->data_report[24+i*9 + 2];
            output->ir_tracking[2+i].position.y |= ((t >> 6) & 0b11) << 8;
            output->ir_tracking[2+i].position.x |= ((t >> 4) & 0b11) << 8;
            output->ir_tracking[2+i].size = t & 0b1111;
        }
    }

    // IR visibility check
    for(int i = 0; i < 4; i++) {
        // 0xFF data
        output->ir_tracking->visible = output->ir_tracking->position.x != 0b1111111111;
    }
}

void wiimotes_initialize() {
    memset(WIIMOTES, 0, sizeof(WIIMOTES));

    // Sensor Bar On
    gpio_write(GPIO_SENSOR_BAR, true);

    // Load settings
    wiimote_sys_phrase_settings();

    bluetooth_driver_t driver = {
        .driver_id = BLUETOOTH_DRIVER_ID_WIIMOTE,
        .filter_device = wiimote_hid_driver_filter,
        .initialize_device = wiimote_hid_driver_initialize,
        .free_device = wiimote_hid_driver_free_device
    };

    bltools_register_driver(driver);

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

    if(output->present & WIIMOTE_PRESENT_IR) {
        wiimote_update_ir(raw_data, output);
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

int wiimote_set_reporting(wiimote_t* wiimote, int present) {
    uint8_t reporting_mode = 0x0;
    // Search for each mode, assuming the first listed modes are more favorable
    // Over the later ones, first mode matching all present bits
    for(int i = 0; i < sizeof(wiimote_present_preferred_table) / sizeof(wiimote_present_preferred_table[0]); i++) {
        uint8_t m = wiimote_present_preferred_table[i];
        int p = wiimote_present_lookup_table[m];

        // If a bit is set in the present, but not in this entry
        // then thats bad, but if were all good, we can use this mode
        if((present & ~p) == 0) {
            reporting_mode = m + 0x30;
        }
    }

    if(reporting_mode == 0x0) {
        return BLERROR_ARGUMENT;
    }

    return wiimote_hid_set_report(wiimote->driver, reporting_mode, true);
}