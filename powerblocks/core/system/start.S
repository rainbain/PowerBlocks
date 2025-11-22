/**
 * @file start.s
 * @brief Access parts of the base system.
 *
 * Access parts of the base system. Like time and such.
 *
 * @author Samuel Fitzsimons (rainbain), Michael Garofalo (Techflash)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

    .global _start
    .section .init
#include "cpu.h"

# NOTE: Reminder that "bit [n]" follows the official PowerPC naming scheme throughout this code  - reversed compared to how one may think about them.  "bit 0 is set" = 0x80000000.  "bit 31 is set" = 0x00000001.

_start:
	b initialize

.global system_argv
    # ARGV Storage, our Magic, argv magic + data
    # Its a very weird system.
	.long	0x5f617267
system_argv:
	.long 0, 0, 0, 0, 0, 0

# We're here!  Let's get a few things knocked out right off the bat (heh, BATs)
# 1. Make sure we're in real-mode (IR and DR off)
#
# 2. Map [I/D]BAT[0/1], 256M @ 0x00000000 (uncached and cached), which gives us:
#    - MEM1
#    - Flipper registers (0x0c000000)
#    - Hollywood and Latte registers (0x0d000000)
#    - Hollywood and Latte registers mirror (0x0d800000)
#
# 3. Map [I/D]BAT[2/3], 256M @ 0x00000000 (uncached and cached), which gives us MEM2
#
# 4. Switch to virtual memory (MSR[IR] = 1, MSR[DR] = 1) with our new known map
#
# 5. Zero BSS
#
# 6. Set up a stack
#
# 7. Zero out the (non-SP) registers
#
# 8. Call into the C init functions
initialize:
	# Load 0 with address of routine to do BATs setup
	lis 0, setup_BATs@h
	ori 0, 0, setup_BATs@l

	# Clear bit 0, since we want the physical address.  This binary is linked assuming virtual addresses in .text, so 0 = 0x8xxxxxxx right now, which is no good.  Clear bit 0 to get a real MEM1-physical address.
	lis 3, 0x7fff
	ori 3, 3, 0xffff
	and 0, 0, 3

	# store it into SRR0 (return address)
	mtsrr0 0

	# get MSR value
	mfmsr 0

	# clear IR (26) and DR (27) bits - in between 28 and 25
	rlwinm 0, 0, 0, 28, 25

	# move it into SRR1 (MSR value)
	mtsrr1 0

	# let's do this thing, in real-mode now
	rfi

setup_BATs:
	# Hello from real-mode!

	# First, lets clear anything that may be in the segment registers (SRs)
	# We want the T (0) bit to be set, so that they basically don't matter.
	lis 0, 0x8000
	mtsr 0, 0
	mtsr 1, 0
	mtsr 2, 0
	mtsr 3, 0
	mtsr 4, 0
	mtsr 5, 0
	mtsr 6, 0
	mtsr 7, 0
	mtsr 8, 0
	mtsr 9, 0
	mtsr 10, 0
	mtsr 11, 0
	mtsr 12, 0
	mtsr 13, 0
	mtsr 14, 0
	mtsr 15, 0

	# Now let's set up the BATs, we want:
	#  - [I/D]BAT0: 256MB @ 0x00000000, cached, R/W,
	#               virtual @ 0x80000000
	#
	#  - [I/D]BAT1: 256MB @ 0x00000000, UNcached, R/W,
	#               virtual @ 0xc0000000
	#
	# This is more or less the "standard" GameCube/Wii memory layout.
	# TODO: For GameCube, mapping MEM2 is pretty silly since it doesn't
	# actually exist.  May want to make that conditional if it's ever
	# decided to expand to GameCube.

	# [I/D]BAT0L: BPRN=0x00000000, WIMG=0000 (cached), PP=RW
	li 3, 0x2
	
	# [I/D]BAT0U: BEPI=0x80000000, BL=256MB, VS=1, VP=1
	lis 4, 0x8000
	ori 4, 4, 0x1fff

	# Set them
	mtspr IBAT0L, 3
	mtspr DBAT0L, 3
	mtspr IBAT0U, 4
	mtspr DBAT0U, 4


	# [I/D]BAT1L: BPRN=0x00000000, WIMG=0101 (uncached, guarded (it's customary to do MMIO here, and largely not actually care about memory access)), PP=RW
	li 3, 0x2a
	
	# [I/D]BAT1U: BEPI=0xc0000000, BL=256MB, VS=1, VP=1
	lis 4, 0xc000
	ori 4, 4, 0x1fff

	# Set them
	mtspr IBAT1L, 3
	mtspr DBAT1L, 3
	mtspr IBAT1U, 4
	mtspr DBAT1U, 4

	# [I/D]BAT2L: BPRN=0x10000000, WIMG=0000 (cached), PP=RW
	lis 3, 0x1000
	ori 3, 3, 0x0002
	
	# [I/D]BAT2U: BEPI=0x00000000, BL=256MB, VS=1, VP=1
	lis 4, 0x9000
	ori 4, 4, 0x1fff

	# Set them
	mtspr IBAT2L, 3
	mtspr DBAT2L, 3
	mtspr IBAT2U, 4
	mtspr DBAT2U, 4


	# [I/D]BAT3L: BPRN=0x10000000, WIMG=0101 (uncached, guarded (it's customary to do MMIO here, and largely not actually care about memory access)), PP=RW
	lis 3, 0x1000
	ori 3, 3, 0x002a
	
	# [I/D]BAT3U: BEPI=0xd0000000, BL=256MB, VS=1, VP=1
	lis 4, 0xd000
	ori 4, 4, 0x1fff

	# Set them
	mtspr IBAT3L, 3
	mtspr DBAT3L, 3
	mtspr IBAT3U, 4
	mtspr DBAT3U, 4

	# enable and invalidate the caches
	mfspr 0, HID0
	ori 0, 0, HID0_ICE | HID0_ICFI | HID0_DCE | HID0_DCFI
	mtspr HID0, 0

	# flush the changes
	sync
	isync

	# prepare to flip IR and DR on
	mfmsr 0
	ori 0, 0, MSR_IR | MSR_DR
	mtsrr1 0

	# Alright, we've made the hardware sane, lets get back to real code
	lis 0, clear_BSS@h
	ori 0, 0, clear_BSS@l
	mtsrr0 0

	rfi # no need to flip address bits - it's already correct

clear_BSS:
	# We now have virtual memory flipped *on*,
	# only a few more steps and we can get out of manual asm hell!
	# Next up we need to clear the BSS.
	#
	# Asumptions:
	# - __bss_start and __bss_end are 4-byte aligned
	# - __bss_end is at least 4 bytes after __bss_start
	#
	# Registers:
	# - 0: zero
	# - 3: bss start / current bss pointer
	# - 4: bss end
	li 0, 0
	lis 3, __bss_start@h
	ori 3, 3, __bss_start@l
	lis 4, __bss_end@h
	ori 4, 4, __bss_end@l

bss_loop:
	# store 0 at 3
	stw 0, 0(3)

	# move forward 4 bytes
	addi 3, 3, 4

	# are we done?
	cmplw 3, 4

	# yes!
	bge setup_stack

	# nope
	b bss_loop

setup_stack:
	# set 1 (sp) to the top of the stack (end of the heap)
	lis 1, __heap_end@h
	ori 1, 1, __heap_end@l

setup_tls:
	# set 2 (TLS pointer) to the start of TLS
	lis 2, __tls_start@h
	ori 2, 2, __tls_start@l

clear_regs:
	li 0, 0
	# don't clear 1, or 2, we just set them above
	li 3, 0 # args for init would go heere if it took any, but it doesn't.
	li 4, 0
	li 5, 0
	li 6, 0
	li 7, 0
	li 8, 0
	li 9, 0
	li 10, 0
	li 11, 0
	li 12, 0
	li 13, 0
	li 14, 0
	li 15, 0
	li 16, 0
	li 17, 0
	li 18, 0
	li 19, 0
	li 20, 0
	li 21, 0
	li 22, 0
	li 23, 0
	li 24, 0
	li 25, 0
	li 26, 0
	li 27, 0
	li 28, 0
	li 29, 0
	li 30, 0
	li 31, 0

final_init:
    bl __libc_init_array

    bl system_initialize
trap:
    b trap

	# BUGFIX: So apparently Dolphin will not detect it as a wii executable when running it directly
	# unless it sees code touching HID4. So you need to fake it to make sure it thinks its a wii
	# executable.
	.long 0x7c13fba6
.section .note.GNU-stack
_note:

