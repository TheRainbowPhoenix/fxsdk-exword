.section .text.start,"ax"
.global _start
_start:
    /* Standalone demo; keep the loader-provided stack. */
    mov.l .Lmain,r0
    jsr @r0
    nop
.Lhalt:
    bra .Lhalt
    nop
.balign 4
.Lmain: .long _main
