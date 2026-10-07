// ++C C Runtime Library (libminicrt) | Platform (FreeBSD x86_64) - thread primitives
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

.text
.global __xxc_unmapself
.type __xxc_unmapself, @function
__xxc_unmapself:
    movl $73, %eax
    syscall
    xorl %edi, %edi
    movl $431, %eax
    syscall
    hlt
.size __xxc_unmapself, . - __xxc_unmapself

.section .note.GNU-stack,"",%progbits
