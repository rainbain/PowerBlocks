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

// Inserts a b <ABSOLUTE ADDRESS> instruction at the givin address
void exception_install_branch(uint32_t address, uint32_t handler) {
    volatile uint32_t* d = (volatile uint32_t*)address;

    int32_t offset = (int32_t)(handler - address);

    // Branch, address, absolute
    uint32_t instruction = (18 << 26) | (offset & 0x03FFFFFC);

    *d = instruction;
}


void exceptions_install_vector() {
    // Flush caches
    system_flush_dcache((void*)0x80000100, 0x800);
    system_invalidate_icache((void*)0x80000100, 0x800);

    // Install branch instructions
    exception_install_branch(0x80000100, (uint32_t)&exceptions_vector_reset);
    //exception_install_branch(0x80000200, (uint32_t)&exceptions_vector_machine_check);
    //exception_install_branch(0x80000300, (uint32_t)&exceptions_vector_dsi);
    //exception_install_branch(0x80000400, (uint32_t)&exceptions_vector_isi);
    exception_install_branch(0x80000500, (uint32_t)&exceptions_vector_external);
    //exception_install_branch(0x80000600, (uint32_t)&exceptions_vector_alignment);
    //exception_install_branch(0x80000700, (uint32_t)&exceptions_vector_program);
    //exception_install_branch(0x80000800, (uint32_t)&exceptions_vector_fpu_unavailable);
    exception_install_branch(0x80000900, (uint32_t)&exceptions_vector_decrementer);

    // Sync to make sure changes are good
    SYSTEM_SYNC();
    SYSTEM_ISYNC();
}

void exception_reset(exception_context_t* context) {
}

void exception_decrementer(exception_context_t* context) {

    SYSTEM_SET_DEC(SYSTEM_S_TO_TICKS(1));
}