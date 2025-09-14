/**
 * @file wiimote_hid.c
 * @brief Bluetooth Instance / Protocol Of The WiiMote
 *
 * Contains the code to actually communicate with wiimotes.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

//
// I was able to get all the information used
// to make this implementation from https://wiibrew.org/wiki/Wiimote
// Its a pretty much complete documentation on this
// Missing bits can be inferred.
// (Very thankful to the authors for there work here.)
//

#include "wiimote_hid.h"

#include "wiimote.h"
#include "wiimote_log.h"

#include <string.h>
#include <stdlib.h>

#define WIIMOTE_HID_INPUT_REPORT  0xA1
#define WIIMOTE_HID_OUTPUT_REPORT 0xA2

#define WIIMOTE_REPORT_LEDS                    0x11 // Set the leds on the controller
#define WIIMOTE_REPORT_REPORT_MODE             0x12 // Used to make/configure reports
#define WIIMOTE_REPORT_STATUS                  0x15 // Request status information
#define WIIMOTE_REPORT_STATUS_INFO             0x20 // Reply to status request, also when it changes states
#define WIIMOTE_REPORT_BUTTONS                 0x30 // Core buttons data
#define WIIMOTE_REPORT_BUTTONS_ACCL            0x31 // Core Buttons + Accelerometer
#define WIIMOTE_REPORT_BUTTONS_EXT8            0x32 // Core Buttons + 8 Extension Bytes
#define WIIMOTE_REPORT_BUTTONS_ACCL_IR12       0X33 // Core Buttons + Accelerometer + 8 Extension Bytes
#define WIIMOTE_REPORT_BUTTONS_EXT19           0X34 // Core Buttons + 19 Extension Bytes
#define WIIMOTE_REPORT_BUTTONS_ACCL_EXT16      0x35 // Core Buttons + Accelerometer + 16 Extension Bytes
#define WIIMOTE_REPORT_BUTTONS_IR10_EXT9       0x36 // Core Buttons + 10 IR Bytes + 9 Extension Bytes
#define WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6 0x37 // Core Buttons + Accelerometer + 10 IR Bytes + 6 Extension Bytes
#define WIIMOTE_REPORT_EXT21                   0x3D // 21 Extension Bytes
#define WIIMOTE_REPORT_INTERLEAVED_A           0x3E // Interleaved Buttons and Accelerometer with 36 IR
#define WIIMOTE_REPORT_INTERLEAVED_B           0x3F // Other end of Interleaved Data

static void wiimote_handle_status_report(wiimote_hid_t* wiimote, const uint8_t* report, size_t length) {
    // Format:
    // BB BB LF 00 00 VV
    // BBBB = Core Button Data
    // L = LED State
    // F = Battery Level Flags
    // 00 00
    // VV = Battery Level

    if(length < 6) {
        WIIMOTE_LOG_ERROR("Status report too small! %d", length);
    }

    
    xSemaphoreTake(wiimote->internal_state_lock, portMAX_DELAY);
    wiimote_raw_t* state = &wiimote->internal_state;
    state->core_state.core_buttons = ((uint16_t)report[0] << 8) | (report[1]);
    state->core_state.flags = report[2];
    state->core_state.battery_level = report[5];
    xSemaphoreGive(wiimote->internal_state_lock);

    // Reenable reporting.
    wiimote_hid_set_report(wiimote, wiimote->set_report_mode);
}

static void wiimote_handle_data_report(wiimote_hid_t* wiimote, uint8_t report_type, const uint8_t* report, size_t length) {
    // Size of each report type starting from 0x30
    static const uint8_t report_lengths[] = {
        2, 5, 10, 17, 21, 21, 21, 21, 0, 0, 0, 0, 0, 21, 21, 21
    };

    int report_length = report_lengths[report_type - 0x30];

    if(length < report_length) {
        WIIMOTE_LOG_ERROR("Status report too small! %d for report %02X", length, report_type);
    }
    
    xSemaphoreTake(wiimote->internal_state_lock, portMAX_DELAY);
    wiimote_raw_t* state = &wiimote->internal_state;
    if(report_type != 0x3F) {
        state->report_type = report_type;
        memcpy(state->data_report, report, report_length);
    } else {
        // Interlace report. Super fun wacky weird
        memcpy(state->data_report + 21, report, report_length);
    }
    xSemaphoreGive(wiimote->internal_state_lock);
}


// Called whenever data comes in for a wiimote
static void wiimote_on_report_in(void* channel_v, void* wiimote_v) {
    l2cap_channel_t* channel = (l2cap_channel_t*)channel_v;
    wiimote_hid_t* wiimote = (wiimote_hid_t*)wiimote_v;

    // Receive Report
    uint8_t payload[64];
    int payload_length = l2cap_receive_channel(channel, payload, sizeof(payload));

    // Make sure we did not drop data
    if(payload_length < 3 || payload_length > sizeof(payload)) {
        WIIMOTE_LOG_ERROR("Data drop detected. %d", payload_length);
        return;
    }

    // We only expect report in signals
    if(payload[0] != WIIMOTE_HID_INPUT_REPORT) {
        WIIMOTE_LOG_ERROR("Received unknown HID code: %02X", payload[0]);
        return;
    }

    // Pointer to report data
    const uint8_t* report = payload + 2;
    int report_length = payload_length - 2;
    uint8_t report_type = payload[1];

    switch(report_type) {
        case WIIMOTE_REPORT_STATUS_INFO:
            wiimote_handle_status_report(wiimote, report, report_length);
            break;
        case WIIMOTE_REPORT_BUTTONS:
        case WIIMOTE_REPORT_BUTTONS_ACCL:
        case WIIMOTE_REPORT_BUTTONS_EXT8:
        case WIIMOTE_REPORT_BUTTONS_ACCL_IR12:
        case WIIMOTE_REPORT_BUTTONS_EXT19:
        case WIIMOTE_REPORT_BUTTONS_ACCL_EXT16:
        case WIIMOTE_REPORT_BUTTONS_IR10_EXT9:
        case WIIMOTE_REPORT_BUTTONS_ACCEL_IR10_EXT6:
        case WIIMOTE_REPORT_EXT21:
        case WIIMOTE_REPORT_INTERLEAVED_A:
        case WIIMOTE_REPORT_INTERLEAVED_B:
            wiimote_handle_data_report(wiimote, report_type, report, report_length);
            break;
        default:
            WIIMOTE_LOG_ERROR("Unhandled report: %02X", report_type);
            return;
    }
}

// Sets the 4 leds
/// TODO: you better improve this
static int wiimote_hid_set_leds(wiimote_hid_t* wiimote, uint8_t leds) {
    uint8_t payload[3] = {WIIMOTE_HID_OUTPUT_REPORT, WIIMOTE_REPORT_LEDS, leds};

    int ret;
    ret = l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
    if(ret < 0)
        return ret;
    
    return 0;
}

int wiimote_hid_initialize(wiimote_hid_t* wiimote, const hci_discovered_device_info_t* discovery, int slot) {
    // Setup
    wiimote->slot = slot;
    wiimote->internal_state_lock = xSemaphoreCreateMutexStatic(&wiimote->semaphore_data[0]);
    
    // Ask HCI for a handle to this device
    uint16_t handle;
    int ret = hci_create_connection(discovery, &handle);
    if(ret < 0) {
        WIIMOTE_LOG_ERROR("Failed to make HCI connection! %d", ret);
        return ret;
    }

    // Open L2CAP Connection
    ret = l2cap_open_device(&wiimote->device, handle);
    if(ret < 0) {
        WIIMOTE_LOG_ERROR("Failed to make L2CAP connection! %d", ret);
        return ret;
    }

    // TODO: If these fail, close the L2CAP instance correctly

    // Open Control Channel
    // Used for commands (though I dont think it uses any)
    ret = l2cap_open_channel(&wiimote->device, &wiimote->control_channel, 0x11,
        wiimote->wiimote_control_channel_buffer, sizeof(wiimote->wiimote_control_channel_buffer));
    if(ret < 0) {
        WIIMOTE_LOG_ERROR("Failed to make control channel! %d", ret);
        return ret;
    }

    // Open Interrupt Channel
    // Used for data
    ret = l2cap_open_channel(&wiimote->device, &wiimote->interrupt_channel, 0x13,
        wiimote->wiimote_interrupt_channel_buffer, sizeof(wiimote->wiimote_interrupt_channel_buffer));
    if(ret < 0) {
        WIIMOTE_LOG_ERROR("Failed to make interrupt channel! %d", ret);
        return ret;
    }

    // Set up events
    l2cap_set_channel_receive_event(&wiimote->interrupt_channel, wiimote_on_report_in, wiimote);

    wiimote_hid_set_leds(wiimote, 0x10 << (slot & 0b11));
    wiimote_hid_set_report(wiimote, 0x30);

    return 0;
}

void wiimote_hid_close(wiimote_hid_t* wiimote) {

}

int wiimote_hid_set_report(wiimote_hid_t* wiimote, uint8_t report_type) {
    // Interestingly, I believe your supposed to use the set report HID code
    // But newer wiimotes make you just use the data in code.
    uint8_t payload[4] = {WIIMOTE_HID_OUTPUT_REPORT, WIIMOTE_REPORT_REPORT_MODE, 0x00, report_type};

    wiimote->set_report_mode = report_type;

    int ret;
    ret = l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
    if(ret < 0)
        return ret;

    return 0;
}


// White lists of accepted parameters for a remote to connect.
// Only any one of the checks has to pass for it to try and connect.
static uint32_t wiimote_whitelist_cods[] = {
    0x480400
};

static const char* wiimote_whitelist_names[] = {
    "Nintendo RVL-CNT-01",
    "Nintendo RVL-CNT-01-TR" // New wii remote plus
};

bool wiimote_hid_driver_filter(const hci_discovered_device_info_t* device, const char* device_name) {
    uint32_t cod = device->class_of_device;
    
    // Check for a valid COD
    for(int i = 0; i < sizeof(wiimote_whitelist_cods) / sizeof(wiimote_whitelist_cods[0]); i++) {
        if(cod == wiimote_whitelist_cods[i]) {
            WIIMOTE_LOG_INFO("COD check passed.");
            return true;
        }
    }

    // Check for a valid name then
    for(int i = 0; i < sizeof(wiimote_whitelist_names) / sizeof(wiimote_whitelist_names[0]); i++) {
        if(strcmp((const char*)device_name, wiimote_whitelist_names[i]) == 0) {
            WIIMOTE_LOG_INFO("COD check failed, name passed.");
            return true;
        }
    }

    WIIMOTE_LOG_DEBUG("COD and Name check failed, device rejected.");
    return false;
}

// Called to initiate a driver
void* wiimote_hid_driver_initialize(const hci_discovered_device_info_t* device) {
    // Find a slot, make sure we have that
    int slot = wiimote_find_empty_slot();
    if(slot < 0)
        return NULL;
    
    wiimote_hid_t* driver = malloc(sizeof(*driver));
    if(driver == NULL)
        return NULL;
    
    int ret;
    ret = wiimote_hid_initialize(driver, device, slot);
    if(ret < 0) {
        free(driver);
        return NULL;
    }

    WIIMOTES[slot].driver = driver;

    return driver;
}

// Called when the device needs to go
void wiimote_hid_driver_free_device(void* instance) {

}