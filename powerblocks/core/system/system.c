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

void system_delay_int(uint64_t ticks) {
    uint64_t stop = system_get_time_base_int() + ticks;

    while(system_get_time_base_int() < stop);
}