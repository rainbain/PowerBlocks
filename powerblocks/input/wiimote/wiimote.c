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
#include "powerblocks/core/utils/macro.h"

#include "wiimote_hid.h"
#include "wiimote_sys.h"
#include "wiimote_log.h"

#include <string.h>

const char* WIIMOTE_TAG = "WIIMOTE";

wiimote_t WIIMOTES[WIIMOTE_MAX_REMOTES];

// Offset of the sensor bar to the TV, based on placement when loading system settings
static vec2i sensor_bar_offset;

// Lookup table of what features are present in each reporting mode
static const int wiimote_present_lookup_table[] = {
    // WIIMOTE_REPORT_BUTTONS
    WIIMOTE_PRESENT_BUTTONS,

    // WIIMOTE_REPORT_BUTTONS_ACCL
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER,

    // WIIMOTE_REPORT_BUTTONS_EXT8
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_EXTENSION,

    // WIIMOTE_REPORT_BUTTONS_ACCL_IR12
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_IR_EXTENDED,

    // WIIMOTE_REPORT_BUTTONS_EXT19
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_EXTENSION,

    // WIIMOTE_REPORT_BUTTONS_ACCL_EXT16
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_EXTENSION,

    // WIIMOTE_REPORT_BUTTONS_IR10_EXT9
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_EXTENSION,

    // WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6
    WIIMOTE_PRESENT_BUTTONS | WIIMOTE_PRESENT_ACCELEROMETER | WIIMOTE_PRESENT_IR | WIIMOTE_PRESENT_EXTENSION,

    // Unused
    0, 0, 0, 0, 0,

    // WIIMOTE_REPORT_EXT21
    WIIMOTE_PRESENT_EXTENSION,

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

    wiimote_set_button_helper(&output->buttons, new_state);
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
    /// TODO: I believe float-int conversion are quite bad on PowerPC 750. Check that, and if so, maybe convert it 1 time.
    vec3 pos;
    pos.x = (float)(x - (int)raw_data->calibration.accel_zero[0]) / (float)raw_data->calibration.accel_one[0];
    pos.y = (float)(y - (int)raw_data->calibration.accel_zero[1]) / (float)raw_data->calibration.accel_one[1];
    pos.z = (float)(z - (int)raw_data->calibration.accel_zero[2]) / (float)raw_data->calibration.accel_one[2];
    output->accelerometer.rectangular = pos;

    // Now that we have that go ahead and calculate the spherical coordinates for it.
    output->accelerometer.spherical.x = vec3_magnitude(pos);
    // atan2(0,0) is undefined. If this happens, the result is implementation specific.
    // Usually the result is 0.
    output->accelerometer.spherical.y = atan2f(pos.x, pos.z);
    output->accelerometer.spherical.z = atan2f(pos.y, sqrtf(pos.x * pos.x + pos.z * pos.z));

    if(fabsf(output->accelerometer.spherical.z - 1.0) < 0.1) {
        output->accelerometer.orientation = output->accelerometer.spherical;
    }
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
            int t = raw_data->data_report[start + 2 + i*5];
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
            int t = raw_data->data_report[start + 1 + i*3];
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

            int t = raw_data->data_report[3+i*9 + 2];
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

        // Invert Xs, their mirrored
        output->ir_tracking[i].position.x = 1023 - output->ir_tracking[i].position.x;
        output->ir_tracking[i].bbox_bottom_right.x = 1023 - output->ir_tracking[i].bbox_bottom_right.x;
        output->ir_tracking[i].bbox_top_left.x = 1023 - output->ir_tracking[i].bbox_top_left.x;
    }
}

static float wiimote_cursor_distance(wiimote_t* output) {
    // Similar to wiiuse implementation.
    // Calculates the distance between the first 2 dots.

    int first_dot = -1;
    for(int i = 0; i < 4; i++) {
        if(!output->ir_tracking[i].visible)
            continue;

        if(first_dot == -1) {
            first_dot = i;
            continue;
        }

        vec2i ab = vec2i_sub(output->ir_tracking[i].position, output->ir_tracking[first_dot].position);
        return vec2i_magnitude(ab);
    }

    return 0.0f;
}

static float wiimote_cursor_yaw(float z, float x) {
    // Full credit to wiiuse use
    // This is where I got this calculation from
    return atanf((x - 512.0f) * (z / 1024.0f) / z);
}

static void rotate_dots(wiimote_t* output, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    vec2i center = vec2i_new(512, 384);

    for(int i = 0; i < 4; i++) {
        if(!output->ir_tracking[i].visible)
            continue;
        
        // Center of rotation
        vec2i p = vec2i_sub(output->ir_tracking[i].position, center);
        p.x = (float)p.x * c + (float)p.y * -s;
        p.y = (float)p.x * s + (float)p.y * c;
        output->ir_tracking[i].position = vec2i_add(p, center);
    }
}

static void wiimote_update_cursor(wiimote_t* output) {
    // First off, ideally there is two points. But we should be able to work with more or less. We will need to know how many
    // We will also calculate the center
    vec2i center = vec2i_new(0, 0);
    int visible_points = 0;
    int index = 0;
    for(int i = 0; i < 4; i++) {
        if(output->ir_tracking[i].visible) {
            center = vec2i_add(center, output->ir_tracking[i].position);
            visible_points++;
            index = i;
        }
    }
    center = vec2i_divs(center, visible_points);

    // Get roll if we have it
    float roll = 0.0f;
    if(output->present & WIIMOTE_PRESENT_ACCELEROMETER) {
        // If were like actively accelerating, dont use it.
        if(output->accelerometer.spherical.x < 1.1f) {
            roll = output->accelerometer.spherical.y;
        }
    }

    // No visible points, clear state.
    if(visible_points == 0) {
        output->cursor.known_dots = 0;
        output->cursor.pos = vec2i_new(0, 0);
        output->cursor.z = 0.0f;
        return;
    }

    // 2 or more visible
    if(visible_points >= 2) {
        rotate_dots(output, roll);

        for(int i = 0; i < 4; i++) {
            if(!output->ir_tracking[i].visible) {
                continue;
            }

            // Calculate the side
            output->ir_tracking[i].side = output->ir_tracking[i].position.x > center.x;
        }

        // Same calculations similar to wiiuse for games who use wii motion like it
        output->cursor.distance = wiimote_cursor_distance(output);
        output->cursor.z = 1023.0f - output->cursor.distance;
        output->cursor.yaw = wiimote_cursor_yaw(output->cursor.z, center.x);
    } else if(output->cursor.known_dots > 1) {
        vec2i other_dot;

        if(output->ir_tracking[index].side) { // Right side, other dot on left
            other_dot = vec2i_new(center.x - (int)output->cursor.distance, center.y);
        } else { // Left side, other dot on right
            other_dot = vec2i_new(center.x + (int)output->cursor.distance, center.y);
        }

        // Average them
        center = vec2i_add(center, other_dot);
        center = vec2i_divs(center, 2);
    } else {
        // 1 point
        rotate_dots(output, roll);
    }

    // Update points seen
    output->cursor.known_dots = MAX(visible_points, output->cursor.known_dots);

    // Correct for sensor bar position
    center = vec2i_sub(center, sensor_bar_offset);

    vec2i top_left = vec2i_new(184, 140);
    vec2i bottom_right = vec2i_new(743, 627);

    // Center the coordinates
    vec2i screen_size = vec2i_new(640, 480);

    // Compute IR range
    vec2i ir_range = vec2i_sub(bottom_right, top_left);

    const int FIXED_SHIFT = 16;
    const int FIXED_ONE = 1 << FIXED_SHIFT;

    // Compute scaling factors (screen_size / ir_range) in fixed point
    vec2i scale = vec2i_new(
        (screen_size.x << FIXED_SHIFT) / ir_range.x,
        (screen_size.y << FIXED_SHIFT) / ir_range.y
    );

    // Translate IR coordinates so top_left becomes (0,0)
    vec2i rel = vec2i_sub(center, top_left);

    // Map to screen space using fixed-point math (with rounding)
    vec2i mapped = vec2i_new(
        (rel.x * scale.x + (FIXED_ONE >> 1)) >> FIXED_SHIFT,
        (rel.y * scale.y + (FIXED_ONE >> 1)) >> FIXED_SHIFT
    );

    // Clamp to screen bounds
    if(mapped.x < 0) mapped.x = 0;
    if(mapped.x > screen_size.x) mapped.x = screen_size.x;
    if(mapped.y < 0) mapped.y = 0;
    if(mapped.y > screen_size.y) mapped.y = screen_size.y;

    // Update cursor position
    output->cursor.pos = mapped;
}

static float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

void wiimotes_initialize() {
    memset(WIIMOTES, 0, sizeof(WIIMOTES));

    // Sensor Bar On
    gpio_set_direction(GPIO_SENSOR_BAR, true);
    gpio_write(GPIO_SENSOR_BAR, true);

    // Load settings
    wiimote_sys_phrase_settings();

    if(wiimote_sys_config.sensor_bar_position) {
        // Top of TV
        sensor_bar_offset = vec2i_new(0, 420 / 2 - 100);
    } else {
        sensor_bar_offset = vec2i_new(0, -420 / 2 + 70);
    }

    // Place 
    bluetooth_driver_t driver = {
        .driver_id = BLUETOOTH_DRIVER_ID_WIIMOTE,
        .filter_device = wiimote_hid_driver_filter,
        .initialize_device = wiimote_hid_driver_initialize,
        .free_device = wiimote_hid_driver_free_device
    };

    bltools_register_driver(driver);

    // Connect wiimotes that have been connected in the past
    //wiimote_connect_registered();
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
        wiimote_update_cursor(output);
    }

    if(output->present & WIIMOTE_PRESENT_EXTENSION) {
        // How offset into the data is the extension data.
        int start = 0;
        if(output->present & WIIMOTE_PRESENT_BUTTONS) start += 2;
        if(output->present & WIIMOTE_PRESENT_ACCELEROMETER) start += 3;
        if(output->present & WIIMOTE_PRESENT_IR) start += 10;

        // Number of extension bytes
        size_t extension_bytes = 0;
        switch(raw_data->report_type) {
            case WIIMOTE_REPORT_BUTTONS_EXT8:
                extension_bytes = 8;
                break;
            case WIIMOTE_REPORT_BUTTONS_EXT19:
                extension_bytes = 19;
                break;
            case WIIMOTE_REPORT_BUTTONS_ACCL_EXT16:
                extension_bytes = 16;
                break;
            case WIIMOTE_REPORT_BUTTONS_IR10_EXT9:
                extension_bytes = 10;
                break;
            case WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6:
                extension_bytes = 6;
                break;
            case WIIMOTE_REPORT_EXT21:
                extension_bytes = 21;
                break;
        }
        wiimote_extension_phrase_data(&output->extensions, raw_data->ext_mapper, raw_data->data_report + start, extension_bytes);
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