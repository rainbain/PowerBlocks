/**
 * @file ios.c
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

#include "ios.h"

#include "system.h"
#include "ipc.h"
#include "ios_settings.h"

#include <stdalign.h>
#include <stddef.h>

#define IOS_COMMAND_OPEN   1
#define IOS_COMMAND_CLOSE  2
#define IOS_COMMAND_READ   3
#define IOS_COMMAND_WRITE  4
#define IOS_COMMAND_SEEK   5
#define IOS_COMMAND_IOCTL  6
#define IOS_COMMAND_IOCTLV 7
#define IOS_COMMAND_ASYNC  8

void ios_initialize() {
    ipc_initialize();
    ios_settings_initialize();
}

int ios_open(const char* path, int mode) {
    alignas(32) ipc_message message;

    // Needs to have a valid path
    if(path == NULL)
        return -1;
    
    // Make sure the path is visible to hardware.
    system_flush_dcache((void*)path, IOS_MAX_PATH);

    message.command = IOS_COMMAND_OPEN;
    message.open.path = (const char*)SYSTEM_MEM_PHYSICAL(path);
    message.open.mode = mode;

    return ipc_request(&message);
}

int ios_close(int file_handle) {
    alignas(32) ipc_message message;

    message.command = IOS_COMMAND_CLOSE;
    message.file_handle = file_handle;

    return ipc_request(&message);
}

int ios_read(int file_handle, void* buffer, int size) {
    alignas(32) ipc_message message;

    message.command = IOS_COMMAND_READ;
    message.file_handle = file_handle;
    message.read.address = (void*)SYSTEM_MEM_PHYSICAL(buffer);
    message.read.size = size;

    // Make sure the new data is visible to the cpu.
    // And that it will not flush while doing this. Hints not after.
    system_invalidate_dcache(buffer, size);

    int ret = ipc_request(&message);

    return ret;
}

int ios_write(int file_handle, void* buffer, int size) {
    alignas(32) ipc_message message;

    message.command = IOS_COMMAND_WRITE;
    message.file_handle = file_handle;
    message.write.address = (void*)SYSTEM_MEM_PHYSICAL(buffer);
    message.write.size = size;

    // Make sure the data is visible to hardware.
    system_flush_dcache(buffer, size);

    return ipc_request(&message);
}

int ios_seek(int file_handle, int where, int whence) {
    alignas(32) ipc_message message;

    message.command = IOS_COMMAND_SEEK;
    message.file_handle = file_handle;
    message.seek.where = where;
    message.seek.whence = whence;

    return ipc_request(&message);
}

int ios_ioctl(int file_handle, int ioctl, void* buffer_in, int in_size, void* buffer_io, int io_size) {
    alignas(32) ipc_message message;

    message.command = IOS_COMMAND_IOCTL;
    message.file_handle = file_handle;
    message.ioctl.ioctl = ioctl;
    message.ioctl.address_in = (void*)SYSTEM_MEM_PHYSICAL(buffer_in);
    message.ioctl.size_in = in_size;
    message.ioctl.address_io = (void*)SYSTEM_MEM_PHYSICAL(buffer_io);
    message.ioctl.size_io = io_size;\

    // Make sure the data is visible to hardware.
    system_flush_dcache(buffer_in, in_size);
    system_flush_dcache(buffer_io, io_size);

    int ret = ipc_request(&message);
    if(ret < 0)
        return ret;
    
    // Invalidate io buffer so we can see it
    system_invalidate_dcache(buffer_io, io_size);

    return ret;
}

int ios_ioctlv(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* argv) {
    const int total = in_size + io_size;

    // Local copy for physical pointers
    alignas(32) ios_ioctlv_t physv[total];

    alignas(32) ipc_message message;
    message.command = IOS_COMMAND_IOCTLV;
    message.file_handle = file_handle;
    message.ioctlv.ioctl = ioctl;
    message.ioctlv.argcin = in_size;
    message.ioctlv.argcio = io_size;
    message.ioctlv.pairs = (void*)SYSTEM_MEM_PHYSICAL(physv);

    // Flush and convert pointers to physical in local copy
    for (int i = 0; i < total; i++) {
        system_flush_dcache(argv[i].data, argv[i].size);
        physv[i].data = (void*)SYSTEM_MEM_PHYSICAL(argv[i].data);
        physv[i].size = argv[i].size;
    }

    // Flush the vector table itself
    system_flush_dcache(physv, sizeof(physv));

    int ret = ipc_request(&message);
    if (ret < 0)
        return ret;

    // Invalidate only IO (output) buffers using original virtual pointers
    for (int i = in_size; i < total; i++) {
        system_invalidate_dcache(argv[i].data, argv[i].size);
    }

    return ret;
}
