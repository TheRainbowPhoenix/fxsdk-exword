.section .header,"a"
.ascii "CASIO CY10x ADDIN MODULE FOR RELEASE ROM\0"
.balign 16,0
.long 0x8c18ff80
.long _start
.space 72

.section .text.start,"ax"
.global _start
_start:
    /* Retain CPU mode; mask interrupts before changing stack. */
    stc sr,r0
    mov #0xf0,r1
    extu.b r1,r1
    or r1,r0
    ldc r0,sr
    mov.l .Lstack,r15
    mov.l .Lmain,r0
    jsr @r0
    nop
.Lhalt:
    bra .Lhalt
    nop
.balign 4
.Lstack: .long __stack_top
.Lmain: .long _main
