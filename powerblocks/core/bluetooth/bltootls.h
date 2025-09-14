/**
 * @file bltools.h
 * @brief Bluetooth Tools
 *
 * Implemented for working with bluetooth.
 * Designed to mainstream connecting devices and loading appropriate drivers.
 * The use of this is not required, it just makes using bluetooth easier.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "FreeRTOS.h"

#include <stdbool.h>

#include "powerblocks/core/bluetooth/hci.h"

// Known Driver IDs part of PowerBlocks
#define BLUETOOTH_DRIVER_ID_INVALID 0
#define BLUETOOTH_DRIVER_ID_WIIMOTE 1

typedef struct {
    uint16_t driver_id; // Unique ID to the driver, that can be used to find it.

    // Covered when a device appears to be discoverable,
    // This can be used to filter out discovered devices
    // Return true if the device applys to this driver
    bool (*filter_device)(const hci_discovered_device_info_t* device, const char* device_name);

    // Called once the filter passes, will attempt to add the device
    // Return a non-null reference to the device if successful
    void* (*initialize_device)(const hci_discovered_device_info_t* device);

    // Called when the device is disconnected. Free up the driver
    void (*free_device)(void* instance);
} bluetooth_driver_t;

typedef struct {
    uint16_t driver_id;
    void* instance;
} bluetooth_driver_instance_t;

/**
 * @brief Initializes all bluetooth.
 * 
 * This mass initializes all the bluetooth parts.
 * And begins bltools.
 */
int bltools_initialize();

/**
 * @brief Will register a device driver.
 * 
 * Once a device driver is registered it can be loaded by the discovery or
 * attempted device connections.
 *
 * @param driver Driver you wish to register.
 */
extern void bltools_register_driver(bluetooth_driver_t driver);

/**
 * @brief Locate a driver's config by its ID
 *
 * @param driver_id ID of the driver you are looking for
 */
extern bluetooth_driver_t* bltoots_find_driver_by_id(uint16_t driver_id);

/**
 * @brief Searches for the first compatable driver for a device.
 *
 * Will search for the first registered compatable driver a device
 * returns as allowed.
 * 
 * @param hci_discovered_device_info_t Device connection information from HCI
 * @param device_name Name of the device, used in filtering.
 */
extern bluetooth_driver_t* bltools_find_compatable_driver(const hci_discovered_device_info_t* device, const char* device_name);

/**
 * @brief Loads a driver onto a device
 * 
 * Like bltools_load_compatable_driver, but assumes you have already chosen
 * a compatible driver
 * 
 * @param driver Driver to use
 * @param discovery Device Connection Information
 * 
 * @return Negative if Error
*/
extern int bltools_load_driver(const bluetooth_driver_t* driver, const hci_discovered_device_info_t* device);

/**
 * @brief Loads a driver for a device if it can find it.
 *
 * Will search for and load a device driver if possible.
 * 
 * @param discovery Device Connection Info
 * @param name Device Name
 * 
 * @return Error if negative
 */
extern int bltools_load_compatable_driver(const hci_discovered_device_info_t* device, const char* device_name);

/**
 * @brief Will begin a discovery session and automatically connexts.
 *
 * Will begin searching for discoverable devices.
 * 
 * When it finds a discoverable device, if it can find a driver for it,
 * it will connect to that device using the driver.
 * 
 * Even if HCI ends the discovery session, it will attempt to go on for the set
 * duration.
 * 
 * WARNING: This creates a thread, it will destroy itself once discovery is done.
 * 
 * @param duration Duration in FreeRTOS ticks for discovery to go on for. Max ticks for forever.
 * 
 * @returns Negative if error.
 */
extern int bltools_begin_automatic_discovery(TickType_t duration);