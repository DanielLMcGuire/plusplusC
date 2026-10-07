// ++C C Runtime Library (libminicrt) | Platform (FreeBSD aarch64) - thread primitives
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

.text
.global __xxc_unmapself
.type __xxc_unmapself, %function
__xxc_unmapself:
    mov  x8, #73
    svc  #0
    mov  x0, #0
    mov  x8, #431
    svc  #0
1:  wfe
    b    1b
.size __xxc_unmapself, . - __xxc_unmapself

.section .note.GNU-stack,"",%progbits
