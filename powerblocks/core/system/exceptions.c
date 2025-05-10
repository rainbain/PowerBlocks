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

#include "system.h"

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

// Processor Interface Registers
#define PI_INTSR  (*(volatile uint32_t*)0xCC003000)
#define PI_INTMR  (*(volatile uint32_t*)0xCC003004)

// The IRQ handlers used with the processor interface
static exception_irq_handler_t irq_handlers[14];

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
    //exception_install_branch(0x80000100, (uint32_t)&exceptions_vector_reset);
    //exception_install_branch(0x80000200, (uint32_t)&exceptions_vector_machine_check);
    //exception_install_branch(0x80000300, (uint32_t)&exceptions_vector_dsi);
    //exception_install_branch(0x80000400, (uint32_t)&exceptions_vector_isi);
    exception_install_branch(0x80000500, (uint32_t)&exceptions_vector_external);
    //exception_install_branch(0x80000600, (uint32_t)&exceptions_vector_alignment);
    //exception_install_branch(0x80000700, (uint32_t)&exceptions_vector_program);
    //exception_install_branch(0x80000800, (uint32_t)&exceptions_vector_fpu_unavailable);
    exception_install_branch(0x80000900, (uint32_t)&exceptions_vector_decrementer);

    // Flush caches
    system_flush_dcache((void*)0x80000100, 0x800);
    system_invalidate_icache((void*)0x80000100, 0x800);

    // Sync to make sure changes are good
    SYSTEM_SYNC();
    SYSTEM_ISYNC();

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
}

void exception_external(exception_context_t* context) {
    // Get IRQ cause and mask
    uint32_t irq_cause = PI_INTSR;
    uint32_t irq_mask = PI_INTMR;

    // Only process enabled interrupts
    irq_cause &= irq_mask;

    // Go through each bit.
    for(int i = 0; i < 14; i++) {
        if((irq_cause & (1<<i)) && (irq_handlers[i] != NULL)) {
            irq_handlers[i](i);
        }
    }
}

void exception_decrementer(exception_context_t* context) {
    SYSTEM_SET_DEC(SYSTEM_S_TO_TICKS(1));
}