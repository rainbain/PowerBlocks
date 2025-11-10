/**
 * @file sdio.h
 * @brief IOS SDIO Interface
 *
 * The SD card slot on the wii goes to a SDIO
 * interface accessible through IOS.
 * 
 * This driver is to interface with it.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

// Implementation based on https://wiibrew.org/wiki//dev/sdio.

/**
 * @brief Opens the SDIO interface
 * 
 * Opens the SDIO interface on the wii.
 * 
 * @param device SDIO device file to open, usually /dev/sdio/slot0 (front SD)
 * 
 * @return Negative if Error
 */
extern int sdio_initialize(const char* device);

/**
 * @brief =Closes the driver.
 * 
 * Closes the driver and assisted IOS file handle.
 */
extern void sdio_close();

/**
 * @brief Returns the status of the SD card slot.
 * 
 * @param state Outputted uint32_t state bitfield
 * @return Negative if error.
 */
extern int sdio_get_device_status(uint32_t *state);

/**
 * @brief Resets the SDIO device, and returns RCA.
 * 
 * @param response First 16 bits are the RCA, next 16 bits are stuff bits. Following the SDIO CMD3.
 * @return Negative if error
 */
extern int sdio_reset_device(uint32_t* response);

/**
 * @brief Reads the OC register
 * Reads the value of the SD cards OC register.
 * This contains a bit feild of operating conditions.
 * 
 * @param oc Outputted 32 bit bit feild
 * @return Negative if error.
 */
extern int sdio_read_oc_register(uint32_t* oc);

/**
 * @brief Reads a host controller register.
 * The wii uses a standard SDIO host controller. You may read an
 * arbitrary register of it using this.
 * 
 * @param offset Register offset.
 * @param value Outputted register value
 * @return Negative if error.
 */
extern int sdio_read_hc_register(uint32_t offset, uint32_t* value);


/**
 * @brief Sets a host controller register.
 * The wii uses a standard SDIO host controller. You may set an
 * arbitrary register of it using this.
 * 
 * @param offset Register offset.
 * @param value Outputted register value
 * @return Negative if error.
 */
extern int sdio_set_hc_register(uint32_t offset, uint32_t value);

/**
 * @brief Sends a SD card command.
 * Sends a command and returns a 16 byte response.
 * 
 * @param cmd
 * @param cmd_type
 * @param resp_type
 * @param arg
 * @param data
 * @param block_count
 * @param sector_size
 */