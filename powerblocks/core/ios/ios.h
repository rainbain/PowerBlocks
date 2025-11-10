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

#include "powerblocks/core/system/ipc.h"

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

/// TODO: Explore and properly document what needs alignment and what does not

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
 * The vector array itself does not need alignment
 * since this function makes its own copy of the vector table
 * when converting the addresses to physical.
 * 
 * @param ioctl I/O Command Enum
 * @param buffer_in Input data to command
 * @param in_size Size of data coming in
 * @param buffer_io Output data of command
 * @param io_size Size of output data
 * @return Negative if error.
 */
extern int ios_ioctlv(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* argv);



/**
 * @brief Opens a file on IOS with callback with async.
 * 
 * Opens a file from IOS's unix-like file system.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param path File path to the file to open
 * @param mode File mode to open with. IOS_MODE_READ or IOS_MODE_WRITE.
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Returns the file handle. Negative if error.
 */
extern int ios_open_async(const char* path, int mode, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Closes a file on IOS with async.
 * 
 * Closes a file descriptor made with ios_open.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param file_handle File handle to close
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_close_async(int file_handle, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Read from a file with async.
 * 
 * Read from a file made with ios_open.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param file_handle File Handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_read_async(int file_handle, void* buffer, int size, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Write to a file with async.
 * 
 * Write to a file made with ios_open.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param file_handle File Handle
 * @param buffer Buffer with data to write
 * @param size Number of bytes to write
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_write_async(int file_handle, void* buffer, int size, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Seek to a position in a file with async.
 * 
 * Seek to a position in a file.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param file_handle File Handle
 * @param where How much to offset from a position
 * @param whence Reference point enum for where in the file.
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_seek_async(int file_handle, int where, int whence, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Send control commands to a device with async.
 * 
 * Send control commands to a device.
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param ioctl I/O Command Enum
 * @param buffer_in Input data to command
 * @param in_size Size of data coming in
 * @param buffer_io Output data of command
 * @param io_size Size of output data
 * @param message Buffer to store the ipc message in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_ioctl_async(int file_handle, int ioctl, void* buffer_in, int in_size, void* buffer_io, int io_size, ipc_message* message, ipc_async_handler_t handler, void* params);

/**
 * @brief Send multiple IO ctrl buffers with async.
 * 
 * Send multiple IO ctrl buffers.
 * 
 * The vector array itself does not need alignment
 * since this function makes its own copy of the vector table
 * when converting the addresses to physical.
 * 
 * The callback is called upon compilation from the ISR.
 * Do not take too long from the ISR!
 * 
 * @param ioctl I/O Command Enum
 * @param buffer_in Input data to command
 * @param in_size Size of data coming in
 * @param buffer_io Output data of command
 * @param io_size Size of output data
 * @param argv Vector of arguments.
 * @param message Buffer to store the ipc message in.
 * @param argv_buffer Buffer to store the hardware argv list in.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Negative if error.
 */
extern int ios_ioctlv_async(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* args, ipc_message* message, ios_ioctlv_t* args_buffer, ipc_async_handler_t handler, void* params);