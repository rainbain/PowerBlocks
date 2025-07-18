/**
 * @file port.c
 * @brief FreeRTOS PowerBlocks Port
 *
 * FreeRTOS PowerBlocks Port
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 */

#include "FreeRTOS.h"
#include "task.h"

#include "system/system.h"
#include "system/syscall.h"
#include "system/exceptions.h"

#include <string.h>

#include "utils/crash_handler.h"

static uint32_t entry_point_stack_pointer;

// Used for FreeRTOS enter and exit safe mode to not
// enable interrupts unless already enabled.
uint32_t freertos_isr_enabled;

// The idle thread is statically allocated. So we will need something to provide that memory.
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

BaseType_t xPortStartScheduler( void )
{
    // Set tick time
    SYSTEM_SET_DEC(SYSTEM_TB_CLOCK_HZ/configTICK_RATE_HZ);

    // Enable Interrupts
    //SYSTEM_ENABLE_ISR(true);

    // Begin the first task
    exceptions_start_first_task();

    return pdTRUE;
}

void vPortEndScheduler( void )
{
}

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    pxTopOfStack -= 464/4;
    exception_context_t* task_context = (exception_context_t*)pxTopOfStack;

    memset(task_context, 0, sizeof(*task_context));

    task_context->srr0 = (uint32_t)pxCode; // PC after exception return
    task_context->srr1 = 0x0001B032;       // Exception state, also will enable interrupts when task begins.

    return pxTopOfStack;
}

void vPortYield( void )
{
    SYSCALL_YIELD();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName ) {
    char msg[64];
    snprintf(msg, sizeof(msg), "STACK OVERFLOW: %s", pcTaskName);
    crash_handler_bug_check(msg, NULL);
}
