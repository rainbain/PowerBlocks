/**
 * @file ios.h
 * @brief Input Output System
 *
 * IOS uses the IPC between broadway and starlet
 * to command starlet to interact with some IO.
 * 
 * This includes things like the SD card and wiimotes.
 * All through starlet's unix like file system for device
 * modules. 
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

#define IOS_MODE_READ  1
#define IOS_MODE_WRITE 2

#define IOS_MAX_PATH 64

/**
 * @struct ios_ioctlv_t
 * @brief Used with ios_ioctlv for sending multiple IO ctrl structures.
 * 
 * Used with ios_ioctlv for sending multiple IO ctrl structures.
 */
typedef struct {
    void* data;
    uint32_t size;
} ios_ioctlv_t;

/**
 * @brief Initializes IOS.
 * 
 * Initializes IOS including the IPC.
 */
extern void ios_initialize();

/**
 * @brief Opens a file on IOS
 * 
 * Opens a file from IOS's unix-like file system.
 * 
 * @param path File path to the file to open
 * @param mode File mode to open with. IOS_MODE_READ or IOS_MODE_WRITE.
 * @return Returns the file handle. Negative if error.
 */
extern int ios_open(const char* path, int mode);

/**
 * @brief Closes a file on IOS
 * 
 * Closes a file descriptor made with ios_open.
 * 
 * @param file_handle File handle to close
 * @return Negative if error.
 */
extern int ios_close(int file_handle);

/**
 * @brief Read from a file.
 * 
 * Read from a file made with ios_open.
 * 
 * @param file_handle File Handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return Negative if error.
 */
extern int ios_read(int file_handle, void* buffer, int size);

/**
 * @brief Write to a file.
 * 
 * Write to a file made with ios_open.
 * 
 * @param file_handle File Handle
 * @param buffer Buffer with data to write
 * @param size Number of bytes to write
 * @return Negative if error.
 */
extern int ios_write(int file_handle, void* buffer, int size);

/**
 * @brief Seek to a position in a file.
 * 
 * Seek to a position in a file.
 * 
 * @param file_handle File Handle
 * @param where How much to offset from a position
 * @param whence Reference point enum for where in the file.
 * @return Negative if error.
 */
extern int ios_seek(int file_handle, int where, int whence);

/**
 * @brief Send control commands to a device.
 * 
 * Send control commands to a device.
 * 
 * @param ioctl I/O Command Enum
 * @param buffer_in Input data to command
 * @param in_size Size of data coming in
 * @param buffer_io Output data of command
 * @param io_size Size of output data
 * @return Negative if error.
 */
extern int ios_ioctl(int file_handle, int ioctl, void* buffer_in, int in_size, void* buffer_io, int io_size);

/**
 * @brief Send multiple IO ctrl buffers.
 * 
 * Send multiple IO ctrl buffers.
 * 
 * @param ioctl I/O Command Enum
 * @param buffer_in Input data to command
 * @param in_size Size of data coming in
 * @param buffer_io Output data of command
 * @param io_size Size of output data
 * @return Negative if error.
 */
extern int ios_ioctlv(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* argv);