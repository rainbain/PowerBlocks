/**
 * @file ipc.h
 * @brief Inter Processor Communications
 *
 * IPC interface between Broadway and Starlet.
 * Designed to implement IOS's protocol.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

typedef void (*ipc_async_handler_t)(void* param, int return_value);

typedef struct {
    int command;
    int returned;
    int file_handle;

    union
    {
        struct {
            const char* path;
            int mode;
        } open;

        struct {
            void* address;
            int size;
        } read;

        struct {
            void* address;
            int size;
        } write;

        struct {
            int where;
            int whence;
        } seek;

        struct {
            int ioctl;

            void* address_in;
            int size_in;

            void* address_io;
            int size_io;
        } ioctl;

        struct {
            int ioctl;
            int argcin;
            int argcio;
            void* pairs;
        } ioctlv;

        uint32_t args[5];
    };

    // Used to prevent spurious responses, since this is cleared after the response is used.
    // Since not doing this could lead to stale stack usage.
    uint32_t magic;

    ipc_async_handler_t response_handler;
    void* params;
    
} ipc_message;

/**
 * @brief Initializes the IPC Interface
 * 
 * Usually called through ios_initialize
 *
 * Initializes the IPC interface.
 * Setups the FreeRTOS Mutexes and data structures for
 * communications.
 * 
 */
extern void ipc_initialize();

/**
 * @brief Puts a request in and gets the response.
 * 
 * Puts a request into the IPC interface.
 * Then after its completion, the handler will be called from the interrupt service.
 * 
 * @param message Data structure to the message to send to Starlet.
 * @param handler Handler called from interrupt service upon compilation.
 * @param params Pointer passed to the handler.
 * @return Result as int, negative if error.
 */
extern int ipc_request(ipc_message* message, ipc_async_handler_t handler, void* params);