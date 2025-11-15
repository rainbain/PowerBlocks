/**
 * @file system.c
 * @brief Access parts of the base system.
 *
 * Access parts of the base system. Like time and such.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "system.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log.h"

#include "exceptions.h"
#include "gpio.h"

// Main of the application of the user.
extern void main();

void system_delay_int(uint64_t ticks) {
    uint64_t stop = system_get_time_base_int() + ticks;

    while(system_get_time_base_int() < stop);
}

void* system_aligned_malloc(uint32_t bytes, uint32_t alignment) {
    // Must be a power of 2
    if((alignment & (alignment-1)) != 0)
        return NULL;
    
    uint32_t raw = (uint32_t)malloc(bytes + alignment - 1 + sizeof(void*));
    if(!raw)
        return NULL;
    
    uint32_t aligned = (raw + sizeof(void*) + aligned - 1) & ~(aligned - 1);
    ((void**)aligned)[-1] = (void*)raw;

    return (void*)aligned;
}

void system_aligned_free(void* ptr) {
    if(ptr) {
        free(((void**)ptr)[-1]);
    }
}

void system_initialize() {
    // Ensure interrupts are disabled
    uint32_t level;
    SYSTEM_DISABLE_ISR(level);

    // Arguments
    // Check magic
    //if(argv->magic == 0x5f617267) {
        //memcpy(&system_argv, argv, sizeof(system_argv_t));
    //} else {
        // Empty one
    //    system_argv.magic = 0x5f617267;
    //    system_argv.command_line = NULL;
    //    system_argv.command_line_length = 0;
    //    system_argv.argc = 0;
    //    system_argv.argv = NULL;
    //    system_argv.end_argv = NULL;
    //}

    log_initialize();

    // Install exception handlers
    exceptions_install_vector();

    // Create main task and start scheduler
    xTaskCreate(main, "MAIN", SYSTEM_MAIN_STACK_SIZE, NULL, configMAX_PRIORITIES / 2, NULL);
    vTaskStartScheduler();

    // We do not expect execution to return here.
}

void system_get_boot_path(const char* device, char* buffer, size_t length) {
    if(buffer == NULL || length < 1)
        return;
    
    // Default path
    if(length > 1) {
        buffer[0] = '/';
        buffer[1] = 0;
    } else {
        buffer[0] = 0;
        return;
    }

    // If no command line, then exit
    if(system_argv.command_line == NULL) {
        return;
    }

    const char* cmd = system_argv.command_line;

    // Find colon separating device from path
    const char *colon = strchr(cmd, ':');
    if(!colon)
        return; // Malformed, exit
    
    size_t devlen = colon - cmd;
    if(devlen == 0)
        return;

    char boot_device[16];
    if(devlen >= sizeof(boot_device))
        devlen = sizeof(boot_device) - 1;
    
    memcpy(boot_device, cmd, devlen);
    boot_device[devlen] = 0;

    // Compare device
    if(strcmp(boot_device, device) != 0) {
        return; // Wrong device, leave default "/"
    }

    // Now extract path AFTER the colon
    const char* path_start = colon + 1; // skip the ':'

    // Find last slash (to remove filename)
    const char* last_slash = strrchr(path_start, '/');
    if(!last_slash)
        return; // No slash, malformed
    
    size_t dir_len = (last_slash - path_start) + 1; // include trailing slash
    if(dir_len >= length)
        dir_len = length - 1;

    // Copy only the path portion, skipping "sd:" or "usb:"
    memcpy(buffer, path_start, dir_len);
    buffer[dir_len] = 0;
}
