OUTPUT_ARCH(sh)
ENTRY(_start)
MEMORY
{
    rom (rx) : ORIGIN = 0x70000080, LENGTH = 0x1FF80
    ram (rw) : ORIGIN = 0x7400004c, LENGTH = 0x7FB4
}
SECTIONS
{
    .text : {
        _stext = .;
        KEEP(*(.text.start))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > rom
    .bss (NOLOAD) : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > ram
    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > ram AT>rom
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
