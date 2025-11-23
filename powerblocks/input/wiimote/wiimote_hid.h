/**
 * @file wiimote_hid.h
 * @brief Bluetooth Instance / Protocol Of The WiiMote
 *
 * Contains the code to actually communicate with wiimotes.
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/bluetooth/l2cap.h"
#include "powerblocks/core/bluetooth/hci.h"

#include "powerblocks/input/wiimote/wiimote.h"
#include "powerblocks/input/wiimote/wiimote_extension.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define WIIMOTE_REPORT_LEDS                    0x11 // Set the leds on the controller
#define WIIMOTE_REPORT_REPORT_MODE             0x12 // Used to make/configure reports
#define WIIMOTE_REPORT_ENABLE_CAMERA_CLOCK     0x13 // Enables 24 MHz clock to the internal camera
#define WIIMOTE_REPORT_STATUS                  0x15 // Request status information
#define WIIMOTE_REPORT_WRITE_MEMORY            0x16 // Writes to the internal memory of the wiimote
#define WIIMOTE_REPORT_READ_MEMORY             0x17 // Request to read memory data
#define WIIMOTE_REPORT_ENABLE_CAMERA           0x1A // Activates the camera enable line
#define WIIMOTE_REPORT_STATUS_INFO             0x20 // Reply to status request, also when it changes states
#define WIIMOTE_REPORT_READ_MEMORY_DATA        0x21 // Reply to the read memory report.
#define WIIMOTE_REPORT_ACKNOWLEDGE_OUTPUT      0x22 // Required acknowledgement to write memory report
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

typedef enum {
    WIIMOTE_FLAGS_BATTERY_NEAR_EMPTY = 0X01,
    WIIMOTE_FLAGS_EXTENSION_CONNECTED = 0X02,
    WIIMOTE_FLAGS_SPEAKER_ENABLED = 0X04,
    WIIMOTE_FLAGS_IR_CAMERA_ENABLED = 0X08,
    WIIMOTE_FLAGS_LED1 = 0X10,
    WIIMOTE_FLAGS_LED2 = 0X20,
    WIIMOTE_FLAGS_LED3 = 0X40,
    WIIMOTE_FLAGS_LEF4 = 0X80
} wiimote_flags_t;

// This is the core data structure of the wiimote
// With all your wiimote information and status.
typedef struct {
    struct {
        uint16_t core_buttons; // The main buttons of the wiimote. Only updated during status reports. So don't use.
        wiimote_flags_t flags; // Contains LED and state information
        uint8_t battery_level;
    } core_state;

    struct {
        uint16_t accel_zero[3];
        uint16_t accel_one[3];
    } calibration;

    const wiimote_extension_mapper_t* ext_mapper;

    uint8_t report_type;
    uint8_t data_report[42]; // Max report size is 21, but interleaved allows for 2 to make 1
} wiimote_raw_t;

typedef struct {
    int slot; // Wiimote slot, or -1 if there is not slot its been assigned to.

    l2cap_device_t device;

    l2cap_channel_t control_channel;
    l2cap_channel_t interrupt_channel;

    uint8_t wiimote_control_channel_buffer[16]; // This channel is barely used if at all.
    uint8_t wiimote_interrupt_channel_buffer[256];

    uint8_t set_report_mode;
    uint8_t set_ir_mode;

    // Internal state
    SemaphoreHandle_t internal_state_lock;
    wiimote_raw_t internal_state;

    StaticSemaphore_t semaphore_data[1];
} wiimote_hid_t;

// Closes a connection to a wiimote
extern void wiimote_hid_close(wiimote_hid_t* wiimote);

// Sets what data is reported back
// You usually want to update the IR mode too.
extern int wiimote_hid_set_report(wiimote_hid_t* wiimote, uint8_t report_type, bool update_ir_mode);

// Driver filter callback
// Will allow connection if a valid wiimote and a slot is available.
extern bool wiimote_hid_driver_filter(const hci_discovered_device_info_t* device, const char* device_name);

// Driver filter paired callback
// Will allow connection if this wiimote has been paired to the wii before.
extern bool wiimote_hid_driver_filter_paired(const hci_discovered_device_info_t* device);

// Called to initiate a driver for a newly paired wiimote
extern void* wiimote_hid_driver_initialize_new(const hci_discovered_device_info_t* device);

// Called to initiate a driver for a reconnecting wiimote
extern void* wiimote_hid_driver_initialize_paired(const hci_discovered_device_info_t* device);

// Called when the device needs to go
extern void wiimote_hid_driver_free_device(void* instance);