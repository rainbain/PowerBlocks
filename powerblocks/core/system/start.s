    .global _start
    .section .init

_start:
    bl __libc_init_array
    b main
trap:
    b trap
