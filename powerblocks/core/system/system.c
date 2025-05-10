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

#include <stdbool.h>

#include "exceptions.h"

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

    /// TODO: Update with context switching time
    SYSTEM_SET_DEC(SYSTEM_S_TO_TICKS(1));

    // Force enable interrupts
    SYSTEM_ENABLE_ISR(true);
}