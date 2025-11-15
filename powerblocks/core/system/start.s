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
	b initialize

.global system_argv
    # ARGV Storage, our Magic, argv magic + data
    # Its a very weird system.
	.long	0x5f617267
system_argv:
	.long 0, 0, 0, 0, 0, 0

initialize:
    bl __libc_init_array

    bl system_initialize
trap:
    b trap
