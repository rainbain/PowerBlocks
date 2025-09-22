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

#include "powerblocks/core/bluetooth/blerror.h"

#include "wiimote.h"
#include "wiimote_sys.h"
#include "wiimote_log.h"

#include "FreeRTOS.h"

#include <string.h>
#include <stdlib.h>

#define WIIMOTE_HID_INPUT_REPORT  0xA1
#define WIIMOTE_HID_OUTPUT_REPORT 0xA2

// Memory space 0x04, Registers
#define WIIMOTE_MEMORY_REGISTER_IR        0x04B00030
#define WIIMOTE_MEMORY_REGISTER_IR_BLOCK1 0x04B00000
#define WIIMOTE_MEMORY_REGISTER_IR_BLOCK2 0x04B0001A
#define WIIMOTE_MEMORY_REGISTER_IR_MODE   0x04B00033

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
    wiimote->set_report_mode = 0; // We want it to update this, since its been disabled
    wiimote_hid_set_report(wiimote, wiimote->set_report_mode, false);
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
        case WIIMOTE_REPORT_ACKNOWLEDGE_OUTPUT:
            WIIMOTE_LOG_DEBUG("Acknowledge output report.");
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

    return l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
}

// Writes to the internal memory of the wiimote
// Writes up to 16 bytes
static int wiimote_write_memory(wiimote_hid_t* wiimote, uint32_t address, const uint8_t* data, size_t size) {
    if(size == 0 || size > 16) {
        return BLERROR_ARGUMENT;
    }

    uint8_t payload[] = {WIIMOTE_HID_OUTPUT_REPORT, WIIMOTE_REPORT_WRITE_MEMORY,
        (address >> 24) & 0xFF, (address >> 16) & 0xFF, (address >> 8) & 0xFF, (address >> 0) & 0xFF,
        size & 0xFF,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    memcpy(payload + 7, data, size);

    return l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
}

static int wiimote_initialize_ir_camera(wiimote_hid_t* wiimote) {
    // Physical Camera Enable
    int ret;
    {
        // Clock on
        uint8_t payload[2] = {WIIMOTE_HID_OUTPUT_REPORT, WIIMOTE_REPORT_ENABLE_CAMERA_CLOCK};
        ret = l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
        if(ret < 0)
            return ret;
        
        // Enable line
        payload[1] = WIIMOTE_REPORT_ENABLE_CAMERA;
        ret = l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
        if(ret < 0)
            return ret;
    }

    // Camera Control And Sensitivity
    {
        // Enable the meow controller
        uint8_t payload = 0x08;
        ret = wiimote_write_memory(wiimote, WIIMOTE_MEMORY_REGISTER_IR, &payload, sizeof(payload));
        if(ret < 0)
            return ret;
        
        // Short delay or else the mode is random
        vTaskDelay(50 / portTICK_PERIOD_MS);

        // Pull sensitivity setting from system settings
        uint8_t selected_mode = wiimote_sys_config.ir_sensitivity;
        if(selected_mode > 4) {
            WIIMOTE_LOG_ERROR("Invalid sensitivity mode: %d", selected_mode);
            selected_mode = 0x2; // Good default
        }

        // Register configurations for different sensitivity modes.
        const static uint8_t sensitivity_modes[][11] = {
            {0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0x64, 0x00, 0xfe, 0xfd, 0x05},
            {0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0x96, 0x00, 0xb4, 0xb3, 0x04},
            {0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0xaa, 0x00, 0x64, 0x63, 0x03},
            {0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0xc8, 0x00, 0x36, 0x35, 0x03},
            {0x07, 0x00, 0x00, 0x71, 0x01, 0x00, 0x72, 0x00, 0x20, 0x1f, 0x03}
        };

        const uint8_t* mode_data = sensitivity_modes[selected_mode];
        ret = wiimote_write_memory(wiimote, WIIMOTE_MEMORY_REGISTER_IR_BLOCK1, mode_data, 9);
        if(ret < 0)
            return ret;
        ret = wiimote_write_memory(wiimote, WIIMOTE_MEMORY_REGISTER_IR_BLOCK2, mode_data + 9, 2);
        if(ret < 0)
            return ret;
        
        // Short delay or else the mode is random
        vTaskDelay(50 / portTICK_PERIOD_MS);

        // Now set mode for IR Mode register (threw the set report function)
        wiimote_hid_set_report(wiimote, wiimote->set_report_mode, true);
    }

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
        // This is treated as a "info".
        // Its fine for us to attempt reconnection to devices and fail.
        WIIMOTE_LOG_INFO("Failed to make HCI connection! %d", ret);
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

    // LEDs
    ret = wiimote_hid_set_leds(wiimote, 0x10 << (slot & 0b11));
    if(ret < 0)
        return ret;

    // Default reporting mode
    wiimote_hid_set_report(wiimote, 0x30, false);
    if(ret < 0)
        return ret;

    // Get the camera going
    wiimote_initialize_ir_camera(wiimote);
    if(ret < 0)
        return ret;

    return 0;
}

void wiimote_hid_close(wiimote_hid_t* wiimote) {

}

int wiimote_hid_set_report(wiimote_hid_t* wiimote, uint8_t report_type, bool update_ir_mode) {
    // Only update it if we need to
    int ret;
    if(wiimote->set_report_mode != report_type) {
        // Interestingly, I believe your supposed to use the set report HID code
        // But newer wiimotes make you just use the data in code.
        uint8_t payload[4] = {WIIMOTE_HID_OUTPUT_REPORT, WIIMOTE_REPORT_REPORT_MODE, 0x00, report_type};

        wiimote->set_report_mode = report_type;
        ret = l2cap_send_channel(&wiimote->interrupt_channel, payload, sizeof(payload));
        if(ret < 0)
            return ret;
    }
    
    if(!update_ir_mode)
        return 0;
    
    // Update IR mode register
    uint8_t mode;
    switch(report_type) {
        default:
            mode = 0x01; // Basic
        
        case WIIMOTE_REPORT_BUTTONS_ACCL_IR12:
            mode = 0x03; // Extended
        
        case WIIMOTE_REPORT_INTERLEAVED_A:
        case WIIMOTE_REPORT_INTERLEAVED_B:
            mode = 0x05; // Full
    }

    ret = 0;
    if(wiimote->set_ir_mode != mode) {
        ret = wiimote_write_memory(wiimote, WIIMOTE_MEMORY_REGISTER_IR_MODE, &mode, sizeof(mode));
        if(ret >= 0)
            wiimote->set_ir_mode = mode;
    }
    return ret;
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