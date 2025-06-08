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

#include "exceptions.h"

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

    // Install exception handlers
    exceptions_install_vector();

    // Create main task and start scheduler
    xTaskCreate(main, "MAIN", SYSTEM_MAIN_STACK_SIZE, NULL, configMAX_PRIORITIES / 2, NULL);
    vTaskStartScheduler();

    // We do not expect execution to return here.
}