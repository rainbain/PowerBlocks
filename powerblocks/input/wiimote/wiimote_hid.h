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

#include "FreeRTOS.h"
#include "semphr.h"


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

    // Internal state
    SemaphoreHandle_t internal_state_lock;
    wiimote_raw_t internal_state;

    StaticSemaphore_t semaphore_data[1];
} wiimote_hid_t;

// Attempts to connect to the bluetooth device and setup everything
// Returns a bluetooth error code if failed
extern int wiimote_hid_initialize(wiimote_hid_t* wiimote, const hci_discovered_device_info_t* discovery, int slot);

// Closes a connection to a wiimote
extern void wiimote_hid_close(wiimote_hid_t* wiimote);

extern int wiimote_hid_set_report(wiimote_hid_t* wiimote, uint8_t report_type);

// Driver filter callback
// Will allow connection if a valid wiimote and a slot is available.
extern bool wiimote_hid_driver_filter(const hci_discovered_device_info_t* device, const char* device_name);

// Called to initiate a driver
extern void* wiimote_hid_driver_initialize(const hci_discovered_device_info_t* device);

// Called when the device needs to go
extern void wiimote_hid_driver_free_device(void* instance);