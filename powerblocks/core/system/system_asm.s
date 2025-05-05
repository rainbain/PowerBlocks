/**
 * @file system.h
 * @brief Access parts of the base system.
 *
 * Access parts of the base system. Like time and such.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */
    .global system_get_time_base_int
    .global system_flush_dcache_nosync

system_get_time_base_int:
    mftbu 5
    mftb 4

    # Reread value till we got the upper 2 times, and did not change.
    mftbu 3
    cmpw 5, 3
    bne system_get_time_base_int

    blr

system_flush_dcache_nosync:
    # Return if size is zero
    cmpwi 4, 0
    beqlr

    # Align Start Address
    rlwinm  5, 3, 0, 0xFFFFFFE0

    # Create end address
    # Round up to cover a partial line
    # Then algin end address
    add     6, 3, 4
    addi    6, 6, 31
    rlwinm  6, 6, 0, 0xFFFFFFE0

loop:
    # Flush line
    dcbf    0, 5

    # Iterate
    addi    5, 5, 32
    cmpw    5, 6
    blt     loop

    blr