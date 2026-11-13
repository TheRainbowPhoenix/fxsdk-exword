OUTPUT_ARCH(sh)
ENTRY(_start)
MEMORY
{
  rom (rx) : ORIGIN = 0x70000080, LENGTH = 0x1FF80
  ram (rw) : ORIGIN = 0x7400004c, LENGTH = 0x7FB4
}
PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
  bss PT_LOAD FLAGS(6);
}
SECTIONS
{
  .text : {
    _stext = .;
    KEEP(*(.text.start))
    *(.text) *(.text.*)
    *(.rodata) *(.rodata.*)
    _etext = .;
  } >rom :text
  .bss (NOLOAD) : {
    _sbss = .;
    *(.bss) *(.bss.*) *(COMMON)
    _ebss = .;
  } >ram :bss
  .data : {
    _sdata = .;
    *(.data) *(.data.*)
    _edata = .;
  } >ram AT>rom :data
  /DISCARD/ : {
    *(.comment) *(.note*) *(.eh_frame*)
    *(.gnu.attributes)
  }
}
