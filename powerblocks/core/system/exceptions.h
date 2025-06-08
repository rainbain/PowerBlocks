/**
 * @file exceptions.h
 * @brief Handles System Exceptions
 *
 * Handles system exceptions interrupt vector table.
 * This includes interrupts. (asynchronous exceptions)
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

#include <stdint.h>

/** @def EXCEPTION_IRQ_COUNT
 *  @brief Number of IRQ bits on the system.
 *
 *  Number of IRQ bits on the system.
 */
#define EXCEPTION_IRQ_COUNT 15

/** @def EXCEPTION_PPC_IRQ
 *  @brief Access to Holywoods Interrupt Controller Flags
 *
 *  There appears to be another interrupt controller inside of
 *  hollywood added on the Wii.
 * 
 *  This is needed for acknowledging interrupts through it.
 */
#define EXCEPTION_PPC_IRQ       (*(volatile uint32_t*)0xcd000030)
#define EXCEPTION_PPC_IRQ_MASK  (*(volatile uint32_t*)0xcd000034)

/**
 * @struct exception_context_t
 * @brief Pointer to the stack frame of the saved context during interrupts.
 *
 * Pointer to the stack frame of the saved context during interrupts.
 * Will need to be changed inorder to modify restored registers.
 * Also used to save the context during task switching.
 */
typedef struct {
    uint32_t stack_frame;

    // Stack pointer gets excluded
    uint32_t r0, r2, r3, r4, r5, r6, r7;
    uint32_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint32_t r16, r17, r18, r19, r20, r21, r22, r23;
    uint32_t r24, r25, r26, r27, r28, r29, r30, r31;

    uint32_t sprg3;

    uint32_t cr, lr, ctr, xer, dar;
    uint32_t srr1, srr0;

    uint32_t gqr0, gqr1, gqr2, gqr3;
    uint32_t gqr4, gqr5, gqr6, gqr7;

    double f[32];
    uint64_t ffs;
} exception_context_t;

/**
 * @enum exception_irq_type_t
 * @brief Types of IRQs during external exceptions.
 *
 * Types of IRQs during external exceptions retrievd from the processor interface.
 * Also used to enable/disable interrupts. 
 */
typedef enum {
    EXCEPTION_IRQ_TYPE_GP_RUNTIME,
    EXCEPTION_IRQ_TYPE_RESET_SWITCH,
    EXCEPTION_IRQ_TYPE_DVD,
    EXCEPTION_IRQ_TYPE_SERIAL,
    EXCEPTION_IRQ_TYPE_EXI,
    EXCEPTION_IRQ_TYPE_STREAMING,
    EXCEPTION_IRQ_TYPE_DSP,
    EXCEPTION_IRQ_TYPE_MEMORY,
    EXCEPTION_IRQ_TYPE_VIDEO,
    EXCEPTION_IRQ_TYPE_PE_TOKEN,
    EXCEPTION_IRQ_TYPE_PE_FINISH,
    EXCEPTION_IRQ_TYPE_FIFO,
    EXCEPTION_IRQ_TYPE_DEBUGGER,
    EXCEPTION_IRQ_TYPE_HSP,
    EXCEPTION_IRQ_TYPE_IPC
} exception_irq_type_t;

 /**
 *  @brief Call the context switch at the end of the ISR if true.
 *
 * Call the context switch at the end of the ISR if true.
 * Usually used along side xSemaphoreGiveFromISR such that
 * awoken task are switched into when a ISR wakes a task.
 */
extern int32_t exception_isr_context_switch_needed;

/**
 * @typedef exception_irq_handler_t
 * @brief Function pointer to handle irq exceptions.
 *
 * Function pointer to handle irq exceptions.
 * Use exceptions_install_irq to set it.
 *
 * @param irq The IRQ type its handling.
 */
typedef void (*exception_irq_handler_t)(exception_irq_type_t irq);

 /**
 *  @brief Installs the exception vector into the CPU.
 *
 * Installs the exception vector into the CPU.
 * Called during system_initialize as part of start.s.
 * 
 * Assumes interrupts to be disabled when you do this.
 */
extern void exceptions_install_vector();

 /**
 *  @brief Registers an interrupt handler for an IRQ.
 *
 * Registers an interrupt handler using a IRQ type
 */
extern void exceptions_install_irq(exception_irq_handler_t handler, exception_irq_type_t type);

// Exception handlers called from exceptions_asm.s
extern void exception_reset(exception_context_t* context);
extern void exception_machine_check(exception_context_t* context);
extern void exception_dsi(exception_context_t* context);
extern void exception_isi(exception_context_t* context);
extern void exception_external(exception_context_t* context);
extern void exception_alignment(exception_context_t* context);
extern void exception_program(exception_context_t* context);
extern void exception_fpu_unavailable(exception_context_t* context);
extern void exception_decrementer(exception_context_t* context);
extern void exception_syscall(exception_context_t* context);

// Used as just the tail end of a exception for jumping to the first task
extern void exceptions_start_first_task();