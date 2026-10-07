// ++C C Runtime Library (libminicrt) | Platform (FreeBSD aarch64)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

.text
.global syscall
.type syscall, %function
syscall:
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc #0
    b.cs 1f
    ret
1:  neg x0, x0
    ret
.size syscall, . - syscall

.section .note.GNU-stack,"",%progbits
