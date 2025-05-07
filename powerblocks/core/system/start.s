/**
 * @file start.s
 * @brief Access parts of the base system.
 *
 * Access parts of the base system. Like time and such.
 *
 * @author Samuel Fitzsimons (rainbain)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

    .global _start
    .section .init

_start:
    bl __libc_init_array
    b main
trap:
    b trap
