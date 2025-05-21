/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS Configuration
 *
 * FreeRTOS Configuration
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#pragma once

#include "powerblocks/core/system/system.h"

// How quickly the clock ticks for the
// context change interrupt
#define configCPU_CLOCK_HZ SYSTEM_TB_CLOCK_HZ

// How often it should change threads.
#define configTICK_RATE_HZ 1000

// Yes, a task will get switched out if the timer hits
// And not just till it yields.
#define configUSE_PREEMPTION 1

// Yes, round robin equal priority task
#define configUSE_TIME_SLICING 1

/// TODO: Implement This?
/// For optimized task selection
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

// Do not idle the system tick to save power
#define configUSE_TICKLESS_IDLE 0

// Up to priority 10
#define configMAX_PRIORITIES 10

// Minimum Stack size used by the idle task
#define configMINIMAL_STACK_SIZE 1024

// Max length of the name a task can have
#define configMAX_TASK_NAME_LEN 32

// The time base register is 64 bit
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_64_BITS

// Idle task will yield to lower or equal priority task.
#define configIDLE_SHOULD_YIELD 1

// Hooks into useful functions
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

// Checks for stack overflows, this is slower, but useful
// There is an even slower 2 option for a deeper check
#define configCHECK_FOR_STACK_OVERFLOW 1

// Track performance of tasks
/// TODO: Implement me
#define configGENERATE_RUN_TIME_STATS 0

// FreeRTOS Assertions
#define configASSERT(x) ASSERT(x)

// Feature enable/disable
#define configUSE_TASK_NOTIFICATIONS   1
#define configUSE_MUTEXES              1
#define configUSE_RECURSIVE_MUTEXES    1
#define configUSE_COUNTING_SEMAPHORES  1
#define configUSE_QUEUE_SETS           0
#define configUSE_APPLICATION_TASK_TAG 0

#define configUSE_POSIX_ERRNO 0

#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_uxTaskGetStackHighWaterMark 0
#define INCLUDE_xTaskGetIdleTaskHandle      0
#define INCLUDE_eTaskGetState               0
#define INCLUDE_xTimerPendFunctionCall      0
#define INCLUDE_xTaskAbortDelay             0
#define INCLUDE_xTaskGetHandle              0
#define INCLUDE_xTaskResumeFromISR          1
