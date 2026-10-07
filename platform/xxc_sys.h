// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef XXC_SYS_H
#define XXC_SYS_H

#if defined(__linux__)
    #include "sys_linux.h"
#elif defined(__FreeBSD__)
    #include "sys_freebsd.h"
#else
    #error "xxc_sys.h: unsupported OS"
#endif

#endif /* XXC_SYS_H */
