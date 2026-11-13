OUTPUT_ARCH(sh)
ENTRY(_start)
SECTIONS
{
    . = 0x8c18ff80;
    .header : { KEEP(*(.header)) }
    ASSERT(SIZEOF(.header) == 0x80, "Header must be 128 bytes")
    .text : { *(.text.start) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    . = ALIGN(16);
    .stack (NOLOAD) : { . += 0x4000; __stack_top = .; }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
    ASSERT(_start == 0x8c190000, "Unexpected entry address")
}
