// ++C C Runtime Library (libminicrt) | Platform (FreeBSD x86_64)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

.text
.global _start
.type _start, @function

_start:
    xorl %ebp, %ebp
    popq %rdi
    movq %rsp, %rsi
    andq $-16, %rsp
    call pluspluscBoot

    hlt
.size _start, . - _start

.section .note.tag,"a",%note
.p2align 2
.long 8, 4, 1
.asciz "FreeBSD"
.long 1400000

.section .note.GNU-stack,"",%progbits
