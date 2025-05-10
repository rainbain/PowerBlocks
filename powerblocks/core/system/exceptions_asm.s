/**
 * @file exceptions_asm.s
 * @brief Handles System Exceptions
 *
 * Handles system exceptions interrupt vector table.
 * This includes interrupts. (asynchronous exceptions)
 * 
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

.macro exceptions_context_save
    # At this point the MMU is disabled, so you will have
    # To translate the stack pointer to a physical address by clearing the upper 2 bits
    clrlwi 1, 1, 2

    # Create stack frame
    stwu 1, -192(1)

    # Save r0
    stw 0, 0x04(1)

    # Save remaining registers
    stmw 2, 0x08(1)

    #
    # Save Special Purpose Registers
    #

    # General purpose, used as task context.
    mfspr 0, SPRG3
    stw 0, 0x80(1)

    # Save fun state registers
    mfcr 0
    stw 0, 0x84(1)
    mflr 0
    stw 0, 0x88(1)
    mfctr 0
    stw 0, 0x8C(1)
    mfxer 0
    stw 0, 0x90(1)
    mfdar 0
    stw 0, 0x94(1)

    # MSR and interrupt return address
    mfsrr1 0
    stw 0, 0x98(1)
    mfsrr0 0
    stw 0, 0x9C(1)

    # Graphics Quantization Registers (https://wiibrew.org/wiki/Broadway/Registers)
    mfspr 0, 912
    stw 0, 0xA0(1)
    mfspr 0, 913
    stw 0, 0xA4(1)
    mfspr 0, 914
    stw 0, 0xA8(1)
    mfspr 0, 915
    stw 0, 0xAC(1)
    mfspr 0, 916
    stw 0, 0xB0(1)
    mfspr 0, 917
    stw 0, 0xB4(1)
    mfspr 0, 918
    stw 0, 0xB8(1)
    mfspr 0, 919
    stw 0, 0xBC(1)

    #
    # Disable exceptions, Reenable MMU
    # We load the new MSR value in SRR1, and the new address in SRR0
    # So that the RFI instruction
    #

    # Set bits 4 and 5 in MSR for MMU, clear EE bit, sync
    mfmsr 3
    ori 3, 3, 0x30
    rlwinm 3, 3, 0, 17, 15 # Clear bit 15. Fun rlwinm logic
    mtsrr1 3

    # Load address to trap from
    lis 3, from_trap_\@@h
    ori 3, 3, from_trap_\@@l
    mtsrr0 3

    # Go to trap
    rfi

from_trap_\@:
    # Code from this point on is from virtual memory.
    
    # Translate stack pointer to cached
    oris 1, 1, 0x8000

    # Load R3 with stack frame address
    # This will be the first argument
    addi 3, 1, 192

.endm

.macro exceptions_context_restore
    #
    # Restore Special Purpose Registers
    #

    # Load registers r2 - r31
    lmw 2, 0x08(1)

    # Task context
    lwz 0, 0x80(1)
    mtspr SPRG3, 0

    # State registers
    lwz 0, 0x84(1)
    mtcr 0
    lwz 0, 0x88(1)
    mtlr 0
    lwz 0, 0x8C(1)
    mtctr 0
    lwz 0, 0x90(1)
    mtxer 0
    lwz 0, 0x94(1)
    mtdar 0

    # MSR and interrupt return address
    lwz 0, 0x98(1)
    mtsrr1 0
    lwz 0, 0x9C(1)
    mtsrr0 0

    # GQRs
    lwz 0, 0xA0(1)
    mtspr 912, 0
    lwz 0, 0xA4(1)
    mtspr 913, 0
    lwz 0, 0xA8(1)
    mtspr 914, 0
    lwz 0, 0xAC(1)
    mtspr 915, 0
    lwz 0, 0xB0(1)
    mtspr 916, 0
    lwz 0, 0xB4(1)
    mtspr 917, 0
    lwz 0, 0xB8(1)
    mtspr 918, 0
    lwz 0, 0xBC(1)
    mtspr 919, 0

    # Restore r0
    lwz 0, 0x04(1)

    # Restore stack pointer
    addi 1, 1, 192

    rfi
.endm

// 0x0100
// Called when the reset button is hit.
.global exceptions_vector_reset
exceptions_vector_reset:
    exceptions_context_save
    bl exception_reset
    exceptions_context_restore

// 0x0200
// Hardware faults, generally unrecoverable.
.global exceptions_vector_machine_check
exceptions_vector_machine_check:
    exceptions_context_save
    exceptions_context_restore

// 0x0300
// Data storage interrupt, called when data could not be accessed.
// Kind of like a segfault
.global exceptions_vector_dsi
exceptions_vector_dsi:
    exceptions_context_save
    exceptions_context_restore

// 0x0400
// Same idea as DSI but instruction storage version
.global exceptions_vector_isi
exceptions_vector_isi:
    exceptions_context_save
    exceptions_context_restore

// 0x0500
// Called from hardware via the interrupt pin
.global exceptions_vector_external
exceptions_vector_external:
    exceptions_context_save
    exceptions_context_restore

// 0x0600
// Failed to access memory due to alignment
.global exceptions_vector_alignment
exceptions_vector_alignment:
    exceptions_context_save
    exceptions_context_restore

// 0x0700
// Called when a invalid instruction is hit.
// Could be illegal at that time too.
.global exceptions_vector_program
exceptions_vector_program:
    exceptions_context_save
    exceptions_context_restore

// 0x0800
// Called when the floating point unit is disabled.
.global exceptions_vector_fpu_unavailable
exceptions_vector_fpu_unavailable:
    exceptions_context_save
    exceptions_context_restore

// 0x0900
// Called when a decrementing counter hits zero
.global exceptions_vector_decrementer
exceptions_vector_decrementer:
    exceptions_context_save
    bl exception_decrementer
    exceptions_context_restore