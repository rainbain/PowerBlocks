ENTRY(_start)

PHDRS {
    text   PT_LOAD FLAGS(5);  /* R + X */
    rodata PT_LOAD FLAGS(4);  /* R */
    data   PT_LOAD FLAGS(6);  /* R + W */
    bss    PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS {
    /* Start layout at 0x80004000 */
    . = 0x80004000;

    /* Code segment */
    .init : ALIGN(32) {
        *(.init)
    } :text

    .text : ALIGN(32) {
        *(.text)
    } :text

    /* Read-only data */
    .rodata : ALIGN(32) {
        *(.rodata*)
    } :rodata

    /* Writable data */
    .data : ALIGN(32) {
        *(.data)
    } :data

    /* BSS section — allocated in memory but not in file */
    .bss : ALIGN(32) {
        *(.bss)
        *(COMMON)
    } :bss
}
