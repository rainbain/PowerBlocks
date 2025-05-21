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

#include "syscall.h"

#include "FreeRTOS.h"
#include "task.h"

#include "utils/crash_handler.h"

#include "system.h"

#include <stdio.h>

static uint32_t syscall_yield(exception_context_t* context, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    vTaskSwitchContext();
    return 0;
}

static uint32_t syscall_assert(exception_context_t* context, uint32_t line, uint32_t file_p, uint32_t arg3) {
    const char* file = (const char*)file_p;

    char msg[64];
    snprintf(msg, sizeof(msg), "ASSERT FAILED: %s:%d", file, line);
    crash_handler_bug_check(msg, context);
    return 0;
}


const syscall_handler_t syscall_registry[SYSCALL_REGISTRY_SIZE] = {
    syscall_yield,
    syscall_assert
};