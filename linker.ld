ENTRY(_start)

MEMORY {
    mem1 (rxw) : ORIGIN = 0x80004000, LENGTH = 0x017FC000   /* MEM1 usable region */
    mem2 (rxw) : ORIGIN = 0x90000000, LENGTH = 0x04000000   /* MEM2 full 64MB */
}

PHDRS {
    text   PT_LOAD FLAGS(5);  /* R + X */
    .mem2.text   PT_LOAD FLAGS(5);  /* R + X */
    rodata PT_LOAD FLAGS(4);  /* R */
    data   PT_LOAD FLAGS(6);  /* R + W */
    bss    PT_LOAD FLAGS(6);  /* R + W */
    tls    PT_TLS;
}

SECTIONS {
    /* === MEM1 Sections === */
    .init : ALIGN(32) {
        *(.init)
    } >mem1 :text

    .text : ALIGN(32) {
        *(.text)
    } >mem1 :text

    .rodata : ALIGN(32) {
        *(.rodata*)
    } >mem1 :rodata

    .data.rel.ro : ALIGN(32) {
        *(.data.rel.ro*)
    } >mem1 :data

    .data : ALIGN(32) {
        *(.data)
    } >mem1 :data

    .bss (NOLOAD) : ALIGN(32) {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    } >mem1 :bss

    .tls : ALIGN(32) {
        __tls_start = .;
        *(.tdata .tdata.* .tbss .tbss.*)
        __tls_end = .;
    } >mem1 :data :tls

    /* Heap in MEM1 */
    . = ALIGN(32);
    __heap_start = .;
    __heap_end   = 0x816FE000;   /* reserve 1MB for stack at top of MEM1 */

    /* === MEM2 Sections === */
    /* By the looks of it, loader use mem2 during loading, so we cant put it in the elf. */
    .mem2.text (NOLOAD) : ALIGN(32) {
        *(.mem2 .mem2.text .mem2.text.*)
    } >mem2 :text
}
