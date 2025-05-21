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

#include "exceptions.h"

// Main of the application of the user.
extern void main();

void system_delay_int(uint64_t ticks) {
    uint64_t stop = system_get_time_base_int() + ticks;

    while(system_get_time_base_int() < stop);
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