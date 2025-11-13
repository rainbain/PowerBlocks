/**
 * @file sd.h
 * @brief SD Card System
 *
 * Implements the Wii's SD Card Slot
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include "diskio.h"

/**
 * @brief Initializes the sd card system.
 * 
 * Initializes the SD card system for use in the file system.
 * Must be called before mounting the file system,
 * 
 * but called only ever once, even if the sd card is removed.
 * 
 * @return Negative if error.
*/
extern int sd_initialize();

/**
 * @brief Closes it out
*/
extern void sd_close();

/**
 * @brief FatFS disk_status implementation.
*/
extern DSTATUS sd_disk_status();

/**
 * @brief FatFS disk_initialize implementation.
 * 
 * Called on mount after successful status reading.
*/
extern DSTATUS sd_disk_initialize();

/**
 * @brief FatFS disk_read implementation
 */
extern DRESULT sd_disk_read(BYTE* buff, LBA_t sector, UINT count);

/**
 * @brief FatFS disk_read implementation
 */
extern DRESULT sd_disk_write(const BYTE* buff, LBA_t sector, UINT count);

/**
 * @brief FatFS disk_ioctl implementation.
 */
extern DRESULT sd_disk_ioctl(BYTE cmd, void* buff);