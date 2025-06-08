/**
 * @file exceptions.c
 * @brief Handles System Exceptions
 *
 * Handles system exceptions interrupt vector table.
 * This includes interrupts. (asynchronous exceptions)
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#include "exceptions.h"

#include "FreeRTOS.h"
#include "task.h"

#include "utils/crash_handler.h"

#include "system.h"
#include "syscall.h"

#include <stdint.h>
#include <string.h>

// References from exceptions_asm.s
extern const void* exceptions_vector_reset;
extern const void* exceptions_vector_machine_check;
extern const void* exceptions_vector_dsi;
extern const void* exceptions_vector_isi;
extern const void* exceptions_vector_external;
extern const void* exceptions_vector_alignment;
extern const void* exceptions_vector_program;
extern const void* exceptions_vector_fpu_unavailable;
extern const void* exceptions_vector_decrementer;
extern const void* exceptions_vector_syscall;

// Processor Interface Registers
#define PI_INTSR  (*(volatile uint32_t*)0xCC003000)
#define PI_INTMR  (*(volatile uint32_t*)0xCC003004)

int32_t exception_isr_context_switch_needed;

// The IRQ handlers used with the processor interface
static exception_irq_handler_t irq_handlers[EXCEPTION_IRQ_COUNT];

// From FreeRTOS port.c file
extern void prvTickISR();

// Inserts a b <RELATIVE ADDRESS> instruction at the givin address
void exception_install_branch(uint32_t address, uint32_t handler) {
    volatile uint32_t* d = (volatile uint32_t*)address;

    int32_t offset = (int32_t)(handler - address);

    // Branch, address
    uint32_t instruction = (18 << 26) | (offset & 0x03FFFFFC);

    *d = instruction;
}


void exceptions_install_vector() {
    // Install branch instructions
    exception_install_branch(0x80000100, (uint32_t)&exceptions_vector_reset);
    exception_install_branch(0x80000200, (uint32_t)&exceptions_vector_machine_check);
    exception_install_branch(0x80000300, (uint32_t)&exceptions_vector_dsi);
    exception_install_branch(0x80000400, (uint32_t)&exceptions_vector_isi);
    exception_install_branch(0x80000500, (uint32_t)&exceptions_vector_external);
    exception_install_branch(0x80000600, (uint32_t)&exceptions_vector_alignment);
    exception_install_branch(0x80000700, (uint32_t)&exceptions_vector_program);
    exception_install_branch(0x80000800, (uint32_t)&exceptions_vector_fpu_unavailable);
    exception_install_branch(0x80000900, (uint32_t)&exceptions_vector_decrementer);
    exception_install_branch(0x80000C00, (uint32_t)&exceptions_vector_syscall);

    // Flush caches
    system_flush_dcache((void*)0x80000000, 0x1000);
    system_invalidate_icache((void*)0x80000000, 0x1000);

    // Clear IRQ handlers
    memset(irq_handlers, 0, sizeof(irq_handlers));

    // Disable all processor interface external interrupts
    PI_INTMR = 0;
}

void exceptions_install_irq(exception_irq_handler_t handler, exception_irq_type_t type) {
    irq_handlers[type] = handler;

    // Set or clear enable bit.
    if(handler != NULL) {
        PI_INTMR |= (1<<type);
    } else {
        PI_INTMR &= ~(1<<type);
    }
}

void exception_reset(exception_context_t* context) {
    crash_handler_bug_check("RESET", context);
}

void exception_machine_check(exception_context_t* context) {
    crash_handler_bug_check("MACHINE CHECK", context);
}

void exception_dsi(exception_context_t* context) {
    crash_handler_bug_check("DSI EXCEPTION ON DATA LOAD/STORE", context);
}

void exception_isi(exception_context_t* context) {
    crash_handler_bug_check("ISI EXCEPTION ON INSTRUCTION LOAD", context);
}

void exception_external(exception_context_t* context) {
    // Start with this being off
    exception_isr_context_switch_needed = 0;

    // Get IRQ cause and mask
    uint32_t irq_cause = PI_INTSR;
    uint32_t irq_mask = PI_INTMR;

    // Only process enabled interrupts
    irq_cause &= irq_mask;

    // Go through each bit.
    for(int i = 0; i < EXCEPTION_IRQ_COUNT; i++) {
        if((irq_cause & (1<<i)) && (irq_handlers[i] != NULL)) {
            irq_handlers[i](i);
        }
    }

    if(exception_isr_context_switch_needed != 0) {
        vTaskSwitchContext();
    }
}

void exception_alignment(exception_context_t* context) {
    crash_handler_bug_check("ALIGNMENT", context);
}

void exception_program(exception_context_t* context) {
    crash_handler_bug_check("PROGRAM", context);
}

void exception_fpu_unavailable(exception_context_t* context) {
    crash_handler_bug_check("FPU UNIVALBLE", context);
}

void exception_decrementer(exception_context_t* context) {
    // Set tick time
    SYSTEM_SET_DEC(SYSTEM_TB_CLOCK_HZ/configTICK_RATE_HZ);

    if( xTaskIncrementTick() != pdFALSE ) {
        /* Switch to the highest priority task that is ready to run. */
        vTaskSwitchContext();
    }
}

void exception_syscall(exception_context_t* context) {
    uint32_t syscall_id = context->r0;

    if(syscall_id >= SYSCALL_REGISTRY_SIZE) {
        crash_handler_bug_check("INVALID SYSCALL", context);
        return;
    }

    syscall_handler_t handler = syscall_registry[syscall_id];

    context->r3 = handler(context, context->r3, context->r4, context->r5);
}