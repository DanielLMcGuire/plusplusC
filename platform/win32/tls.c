// ++C C Runtime Library (libminicrt) | Platform (win32)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#define va_start vastart_guard
#include <windows.h>
#undef va_start

#include <def.h>

#if defined(_MSC_VER) && !defined(__clang__)
    #pragma section(".tls",       read, write)
    #pragma section(".tls$ZZZ",   read, write)
    #pragma section(".CRT$XLA",   long, read)
    #pragma section(".CRT$XLZ",   long, read)
    #define XXC_PE_SECTION(name) __declspec(allocate(name))
#else
    #define XXC_PE_SECTION(name) __attribute__((section(name)))
#endif

ULONG _tls_index = 0;

XXC_PE_SECTION(".tls")     XXC_USED char _tls_start = 0;
XXC_PE_SECTION(".tls$ZZZ") XXC_USED char _tls_end   = 0;

XXC_PE_SECTION(".CRT$XLA") XXC_USED PIMAGE_TLS_CALLBACK __xl_a = 0;
XXC_PE_SECTION(".CRT$XLZ") XXC_USED PIMAGE_TLS_CALLBACK __xl_z = 0;

#if defined(_WIN64)
    typedef IMAGE_TLS_DIRECTORY64 xxc_tls_dir_t;
#else
    typedef IMAGE_TLS_DIRECTORY32 xxc_tls_dir_t;
#endif

XXC_PE_SECTION(".rdata$T") XXC_USED
const xxc_tls_dir_t _tls_used = {
    (ULONG_PTR)&_tls_start,
    (ULONG_PTR)&_tls_end,
    (ULONG_PTR)&_tls_index,
    (ULONG_PTR)(&__xl_a + 1),
    0,
    0
};
