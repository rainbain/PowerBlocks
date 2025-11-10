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

#include "system/system.h"
#include "system/ipc.h"
#include "ios_settings.h"

#include "FreeRTOS.h"
#include "semphr.h"

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


typedef struct {
    SemaphoreHandle_t waiter;
    int ret;
    StaticSemaphore_t data;
} ios_waiter_alert_args;

static void ios_alert_waiter(void* param, int return_value) {
    ios_waiter_alert_args* args = (ios_waiter_alert_args*)param;
    args->ret = return_value;
    xSemaphoreGiveFromISR(args->waiter, &exception_isr_context_switch_needed);
}

void ios_initialize() {
    ipc_initialize();
    ios_settings_initialize();
}

int ios_open(const char* path, int mode) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_open_async(path, mode, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_close(int file_handle) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_close_async(file_handle, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_read(int file_handle, void* buffer, int size) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_read_async(file_handle, buffer, size, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_write(int file_handle, void* buffer, int size) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_write_async(file_handle, buffer, size, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_seek(int file_handle, int where, int whence) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_seek_async(file_handle, where, whence, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_ioctl(int file_handle, int ioctl, void* buffer_in, int in_size, void* buffer_io, int io_size) {
    alignas(32) ipc_message message;
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_ioctl_async(file_handle, ioctl, buffer_in, in_size, buffer_io, io_size, &message, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}

int ios_ioctlv(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* argv) {
    alignas(32) ipc_message message;
    alignas(32) ios_ioctlv_t args_buffer[in_size+io_size];
    ios_waiter_alert_args args;
    args.waiter = xSemaphoreCreateCountingStatic(1, 0, &args.data);

    int ret = ios_ioctlv_async(file_handle, ioctl, in_size, io_size, argv, &message, args_buffer, ios_alert_waiter, &args);
    if(ret < 0)
        return ret;
    
    xSemaphoreTake(args.waiter, portMAX_DELAY);
    return args.ret;
}


int ios_open_async(const char* path, int mode, ipc_message* message, ipc_async_handler_t handler, void* params) {
    // Needs to have a valid path
    if(path == NULL)
        return -1;
    
    // Make sure the path is visible to hardware.
    system_flush_dcache((void*)path, IOS_MAX_PATH);

    message->command = IOS_COMMAND_OPEN;
    message->open.path = (const char*)SYSTEM_MEM_PHYSICAL(path);
    message->open.mode = mode;

    return ipc_request(message, handler, params);
}

int ios_close_async(int file_handle, ipc_message* message, ipc_async_handler_t handler, void* params) {
    message->command = IOS_COMMAND_CLOSE;
    message->file_handle = file_handle;

    return ipc_request(message, handler, params);
}

int ios_read_async(int file_handle, void* buffer, int size, ipc_message* message, ipc_async_handler_t handler, void* params) {
    message->command = IOS_COMMAND_READ;
    message->file_handle = file_handle;
    message->read.address = (void*)SYSTEM_MEM_PHYSICAL(buffer);
    message->read.size = size;

    // Make sure the new data is visible to the cpu.
    // And that it will not flush while doing this. Hints not after.
    system_invalidate_dcache(buffer, size);

    return ipc_request(message, handler, params);
}

int ios_write_async(int file_handle, void* buffer, int size, ipc_message* message, ipc_async_handler_t handler, void* params) {
    message->command = IOS_COMMAND_WRITE;
    message->file_handle = file_handle;
    message->write.address = (void*)SYSTEM_MEM_PHYSICAL(buffer);
    message->write.size = size;

    // Make sure the data is visible to hardware.
    system_flush_dcache(buffer, size);

    return ipc_request(message, handler, params);
}

int ios_seek_async(int file_handle, int where, int whence, ipc_message* message, ipc_async_handler_t handler, void* params) {
    message->command = IOS_COMMAND_SEEK;
    message->file_handle = file_handle;
    message->seek.where = where;
    message->seek.whence = whence;

    return ipc_request(message, handler, params);
}

int ios_ioctl_async(int file_handle, int ioctl, void* buffer_in, int in_size, void* buffer_io, int io_size, ipc_message* message, ipc_async_handler_t handler, void* params) {
    message->command = IOS_COMMAND_IOCTL;
    message->file_handle = file_handle;
    message->ioctl.ioctl = ioctl;
    message->ioctl.address_in = (void*)SYSTEM_MEM_PHYSICAL(buffer_in);
    message->ioctl.size_in = in_size;
    message->ioctl.address_io = (void*)SYSTEM_MEM_PHYSICAL(buffer_io);
    message->ioctl.size_io = io_size;\

    // Make sure the data is visible to hardware.
    system_flush_dcache(buffer_in, in_size);
    system_flush_dcache(buffer_io, io_size);

    return ipc_request(message, handler, params);
}

int ios_ioctlv_async(int file_handle, int ioctl, int in_size, int io_size, ios_ioctlv_t* argv, ipc_message* message, ios_ioctlv_t* args_buffer, ipc_async_handler_t handler, void* params) {
    const int total = in_size + io_size;

    message->command = IOS_COMMAND_IOCTLV;
    message->file_handle = file_handle;
    message->ioctlv.ioctl = ioctl;
    message->ioctlv.argcin = in_size;
    message->ioctlv.argcio = io_size;
    message->ioctlv.pairs = (void*)SYSTEM_MEM_PHYSICAL(args_buffer);

    // Flush and convert pointers to physical in local copy
    for (int i = 0; i < total; i++) {
        system_flush_dcache(argv[i].data, argv[i].size);
        args_buffer[i].data = (void*)SYSTEM_MEM_PHYSICAL(argv[i].data);
        args_buffer[i].size = argv[i].size;
    }

    // Flush the vector table itself
    system_flush_dcache(args_buffer, sizeof(ios_ioctlv_t) * total);

    return ipc_request(message, handler, params);
}
