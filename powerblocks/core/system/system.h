/**
 * @file system.h
 * @brief Access parts of the base system.
 *
 * Access parts of the base system. Like time and such.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

/** @def SYSTEM_BUS_CLOCK_HZ
 *  @brief Memory bus clock speed
 *
 *  Clock speed of the memory bus. It also defines the time of the CPU.
 */
#define SYSTEM_BUS_CLOCK_HZ 243000000

/** @def SYSTEM_CORE_CLOCK_HZ
 *  @brief Core clock speed.
 *
 *  Clock speed of the CPU core.
 */
#define SYSTEM_CORE_CLOCK_HZ 729000000

/** @def SYSTEM_TB_CLOCK_HZ
 *  @brief Time base (internal CPU counter) clock speed.
 *
 *  Clock speed of the CPU's internal clock.
 *  It is 1/4th the bus clock speed.
 */
 #define SYSTEM_TB_CLOCK_HZ (SYSTEM_BUS_CLOCK_HZ / 4)

/** @def SYSTEM_US_TO_TICKS
 *  @brief Convert microseconds to time base ticks.
 *
 * Convert microseconds to time base ticks.
 */
#define SYSTEM_US_TO_TICKS(us) (SYSTEM_TB_CLOCK_HZ / 1000000 * (us))

/** @def SYSTEM_MS_TO_TICKS
 *  @brief Convert miliseconds to time base ticks.
 *
 * Convert miliseconds to time base ticks.
 */
#define SYSTEM_MS_TO_TICKS(ms) (SYSTEM_TB_CLOCK_HZ / 1000 * (ms))

/** @def SYSTEM_S_TO_TICKS
 *  @brief Convert seconds to time base ticks.
 *
 * Convert seconds to time base ticks.
 */
#define SYSTEM_S_TO_TICKS(s) (SYSTEM_TB_CLOCK_HZ * (s))

 /** @def SYSTEM_MEM_UNCACHED
 *  @brief Convert a memory address into a uncached virtual address
 *
 *  Converts a cached address into a uncached virtual address.
 *  This is done by setting the MSB nibble.
 */
#define SYSTEM_MEM_UNCACHED(address) (((uint32_t)(address) & 0x0FFFFFFF) | 0xC0000000)

 /** @def SYSTEM_MEM_CACHED
 *  @brief Convert a memory address into a cached virtual address.
 *
 *  Converts a cached address into a cached virtual address.
 *  This is done by setting the MSB nibble.
 */
#define SYSTEM_MEM_CACHED(address) (((uint32_t)(address) & 0x0FFFFFFF) | 0x80000000)

 /** @def SYSTEM_MEM_PHYSICAL
 *  @brief Convert a memory address into a physical address.
 *
 *  Converts a cached address into a physical address.
 *  This is done by setting the MSB nibble.
 */
#define SYSTEM_MEM_PHYSICAL(address) ((uint32_t)(address) & 0x0FFFFFFF)

 /**
 * @brief Gets the time base register of the CPU.
 *
 * Returns the 64 bit internal timer value of the CPU.
 * Ticks up at speed of SYSTEM_TB_CLOCK_HZ.
 * 
 * No interrupts version.
 */
extern uint64_t system_get_time_base_int();

/**
 * @brief Sleeps for a certain number of ticks
 *
 * Waits for a certain number of ticks to elapse.
 * 
 * No interrupts version.
 * 
 * @param ticks Number of ticks to wait for
 */
extern void system_delay_int(uint64_t ticks);

/**
 * @brief Flushes data cache in a range
 *
 * Flushes the CPUs data cache in a range such
 * that all changes from CPU be visible to hardware.
 * 
 * Does not call sync after, so right after this the cache
 * may not be done flushing.
 */
extern void system_flush_dcache_nosync(void* data, uint32_t size);

#include <stdint.h>