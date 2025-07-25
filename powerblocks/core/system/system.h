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

#include "powerblocks/core/system/syscall.h"

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

 /** @def SYSTEM_MAIN_STACK_SIZE
 *  @brief Size of the stack of the application main.
 *
 *  Size of the stack of the application main.
 *  When the system starts, the main() will be a FreeRTOS task
 *  of this stack size.
 */
#define SYSTEM_MAIN_STACK_SIZE (1024*1024*4) // 4 MB

 /** @def SYSTEM_GET_MSR
 *  @brief Gets the value of the MSR register
 *
 *  Gets the value of the MSR register.
 */
#define SYSTEM_GET_MSR(msr) \
    __asm__ __volatile__( \
        "mfmsr %0" : "=r"(msr) \
    );

 /** @def SYSTEM_SET_MSR
 *  @brief Sets the value of the MSR register
 *
 *  Sets the value of the MSR register.
 */
#define SYSTEM_SET_MSR(msr) \
    __asm__ __volatile__( \
        "mtmsr %0" : : "r"(msr) \
    );

 /** @def SYSTEM_GET_DEC
 *  @brief Gets the value of the decrementing register.
 *
 * Gets the value of the decrementing register.
 * When it his zero an interrupt will be triggered. Used for context switching.
 */
#define SYSTEM_GET_DEC(msr) \
    __asm__ __volatile__( \
        "mfdec %0" : "=r"(msr) \
    );

 /** @def SYSTEM_SET_DEC
 *  @brief Sets the value of the decrementing register.
 *
 * Sets the value of the decrementing register.
 * When it his zero an interrupt will be triggered. Used for context switching. 
 */
#define SYSTEM_SET_DEC(msr) \
    __asm__ __volatile__( \
        "mtdec %0" : : "r"(msr) \
    );

 /** @def SYSTEM_DISABLE_ISR
 *  @brief Disables interrupts
 *
 *  Disables interrupts, returns the external interrupt enable bit.
 */
#define SYSTEM_DISABLE_ISR(ee_enabled) \
    do { \
        uint32_t msr; \
        SYSTEM_GET_MSR(msr); \
        ee_enabled = (msr >> 15) & 1; \
        msr &= ~(0x8000); \
        SYSTEM_SET_MSR(msr); \
    } while(0)

 /** @def SYSTEM_ENABLE_ISR
 *  @brief Enables interrupts if ee_enabled is set.
 *
 *  Enables interrupts if ee_enabled is set.
 *  Can be used along with SYSTEM_DISABLE_ISR, to only
 *  reenable interrupts if they were enabled in the first place.
 */
#define SYSTEM_ENABLE_ISR(ee_enabled) \
    if(ee_enabled) { \
        uint32_t msr; \
        SYSTEM_GET_MSR(msr); \
        msr |= 0x8000; \
        SYSTEM_SET_MSR(msr); \
    }

 /** @def SYSTEM_SYNC
 *  @brief Insures all previous instructions have executed.
 *
 *  Insures all previous instructions have executed.
 */
#define SYSTEM_SYNC() \
    __asm__ __volatile__( \
        "sync" \
    );

 /** @def SYSTEM_ISYNC
 *  @brief Refetches any instructions that may have been fetched earlier.
 *
 *  Refetches any instructions that may have been fetched earlier.
 */
#define SYSTEM_ISYNC() \
    __asm__ __volatile__( \
        "isync" \
    );

 /** @def ASSERT
 *  @brief Triggers a crash with debug info if a condition fails.
 *
 *  Triggers a crash with debug info if a condition fails.
 */
#define ASSERT(x) \
    if((x) == 0) { \
        SYSCALL_ASSERT(__LINE__, __FILE__); \
    }

 /** @def ALIGN
 *  @brief Add an alignment attribute to some data.
 *
 *  Used to make a variable memory aligned for hardware
 */
#define ALIGN(x) __attribute__((aligned(x)))

 /** @def SYSTEM_SWITCH_SP
 *  @brief Switches the stack pointer with pxCurrentTCB in a task switch.
 *
 *  Switches the stack pointer with pxCurrentTCB in a task switch.
 *  Do not call this outside of exception handers/syscalls that switch execution context.
 */
#define SYSTEM_SWITCH_SP(x) \
     __asm__ __volatile__ (                      \
        "lis    r3, pxCurrentTCB@ha\n\t"        \
        "lwz    r4, pxCurrentTCB@l(r3)\n\t"     /* r4 = pxCurrentTCB */ \
        "mr     r5, r1\n\t"                     /* r5 = current SP */ \
        "stw    r5, 0(r4)\n\t"                  /* pxCurrentTCB->pxTopOfStack = SP */ \
                                                \
        /* Call vTaskSwitchContext() */         \
        "bl     vTaskSwitchContext\n\t"         \
                                                \
        "lis    r3, pxCurrentTCB@ha\n\t"        \
        "lwz    r4, pxCurrentTCB@l(r3)\n\t"     /* r4 = pxCurrentTCB */ \
        "lwz    r1, 0(r4)\n\t"                  /* SP = pxCurrentTCB->pxTopOfStack */ \
    )



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
extern void system_flush_dcache(void* data, uint32_t size);

/**
 * @brief Invalidates data cache in a range
 *
 * Invalidates the CPUs data cache in a range such
 * that all changes from hardware are visible to the CPU.
 */
extern void system_invalidate_dcache(void* data, uint32_t size);

/**
 * @brief Invalidates the instruction cache in a range.
 *
 * Invalidates the CPUs instruction cache in a range such
 * that all changes from CPU be visible to instruction fetching.
 * 
 * Does not call isync after, so right after this the cache
 * may not be done invalidating.
 */
extern void system_invalidate_icache(void* data, uint32_t size);

/**
 * @brief Allocate memory with alignment.
 * 
 *  Allocate memory aligned to a certain number of bytes.
 *  A lot of hardware requires aligned memory. Usually 32 bytes.
 * 
 *  Must be a power of 2.
 *  
 *  Offsets the pointer by the alignment and stores the original unaligned
 *  pointer before the pointer returned.
 */
extern void* system_aligned_malloc(uint32_t bytes, uint32_t alignment);

/**
 * @brief Free allocated memory with alignment.
 * 
 * Free allocated memory with alignment.
 */
extern void system_aligned_free(void* ptr);

/**
 * @brief Initializes the system.
 *
 * Initializes the system. Called from start.s,
 * so this all happens before the C program starts running.
 */
extern void system_initialize();

#include <stdint.h>