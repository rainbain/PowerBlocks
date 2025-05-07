ENTRY(_start)

PHDRS {
    text   PT_LOAD FLAGS(5);  /* R + X */
    rodata PT_LOAD FLAGS(4);  /* R */
    data   PT_LOAD FLAGS(6);  /* R + W */
    bss    PT_LOAD FLAGS(6);  /* R + W */
    
    tls    PT_TLS;
}

SECTIONS {
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

    /* Read-only data Relocations. */
    .data.rel.ro : ALIGN(32) {
        *(.data.rel.ro*)
    } :data

    /* Writable data */
    .data : ALIGN(32) {
        *(.data)
    } :data

    /* BSS section — allocated in memory but not in file */
    .bss : ALIGN(32) {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    } :bss

    /* TLS section — thread safe storage */
    .tls : ALIGN(32) {
    __tls_start = .;
    *(.tdata .tdata.* .tbss .tbss.*)
    __tls_end = .;
    } :data :tls

    . = ALIGN(32);
    __heap_start = .;
    /* Reserve everything up to 0x817FE000 - 1 MB for stack */
    __heap_end = 0x816FE000;
}
