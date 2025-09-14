/**
 * @file syscall.h
 * @brief Handle syscall exceptions.
 *
 * Handle syscall exceptions, usually for task control
 * and debugging.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include "powerblocks/core/system/exceptions.h"

#include <stdint.h>


/**
 * @typedef syscall_handler_t
 * @brief Function pointer to handle syscalls.
 *
 * Function pointer to handle syscalls.
 *
 * @param context Pointer to the stack frame of the saved context during interrupts.
 * @param arg1 First argument passed to syscall
 * @param arg2 Second argument passed to syscall
 * @param arg3 Third argument passed to syscall
 * 
 * @return Return value of syscall.
 */
typedef uint32_t (*syscall_handler_t)(exception_context_t* context, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#define SYSCALL_ID_YIELD         0
#define SYSCALL_ID_ASSERT        1
#define SYSCALL_ID_OUT_OF_MEMORY 2

#define SYSCALL_REGISTRY_SIZE 3

extern const syscall_handler_t syscall_registry[SYSCALL_REGISTRY_SIZE];


/**  @def SYSCALL
  *  @brief Calls a Syscall with a linux like calling convention.
 *
 *  Calls a Syscall with a linux like calling convention.
 */
#define SYSCALL(num, arg1, arg2, arg3)                           \
({                                                               \
    register uint32_t r0 __asm__("r0") = (num);                  \
    register uint32_t r3 __asm__("r3") = (uint32_t)(arg1);       \
    register uint32_t r4 __asm__("r4") = (uint32_t)(arg2);       \
    register uint32_t r5 __asm__("r5") = (uint32_t)(arg3);       \
    __asm__ volatile (                                           \
        "sc\n\t"                                                 \
        : "+r" (r3)                                              \
        : "r" (r0), "r" (r4), "r" (r5)                           \
        : "cr0", "memory"                                        \
    );                                                           \
    r3; /* return value in r3 */                                 \
});

/**  @def SYSCALL_YIELD
  *  @brief Called for FreeRTOS to task switch the current context.
 *
 * Called for FreeRTOS to task switch the current context.
 * Use taskYIELD for most applications.
 */
#define SYSCALL_YIELD() SYSCALL(SYSCALL_ID_YIELD, 0, 0, 0)

/**  @def SYSCALL_ASSERT
  *  @brief Called for when an assert fails to report the information.
 *
 * Called for when an assert fails to report the information.
 */
#define SYSCALL_ASSERT(line, file) SYSCALL(SYSCALL_ID_ASSERT, line, file, 0)

/**  @def SYSCALL_OUT_OF_MEMORY
  *  @brief Called for when an assert fails to report the information.
 *
 * Called for when an assert fails to report the information.
 */
#define SYSCALL_OUT_OF_MEMORY(line, file) SYSCALL(SYSCALL_ID_OUT_OF_MEMORY, line, file, 0)