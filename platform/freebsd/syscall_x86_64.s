// ++C C Runtime Library (libminicrt) | Platform (FreeBSD x86_64)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

.text
.global syscall
.type syscall, @function
syscall:
    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    movq %r8,  %r10
    movq %r9,  %r8
    movq 8(%rsp), %r9
    syscall
    jc 1f
    ret
1:  negq %rax
    ret
.size syscall, . - syscall

.section .note.GNU-stack,"",%progbits
