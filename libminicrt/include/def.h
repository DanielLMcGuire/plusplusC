// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef DEF_H
#define DEF_H

#define __NORETURN__ while (1) (void)0;

#if defined(__linux__) || defined(__FreeBSD__)
    #define XXC_RAWSYS 1
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define XXC_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define XXC_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define XXC_NOINLINE    __attribute__((noinline))
    #define XXC_NORETURN    __attribute__((noreturn))
    #define XXC_UNUSED      __attribute__((unused))
    #define XXC_USED      __attribute__((used))
#elif defined(_MSC_VER)
    #define XXC_LIKELY(x)   (x)
    #define XXC_UNLIKELY(x) (x)
    #define XXC_NOINLINE    __declspec(noinline)
    #define XXC_NORETURN    __declspec(noreturn)
    #define XXC_UNUSED
    #define XXC_USED
#else
    #define XXC_LIKELY(x)   (x)
    #define XXC_UNLIKELY(x) (x)
    #define XXC_NOINLINE
    #define XXC_NORETURN
    #define XXC_UNUSED
    #define XXC_USED
#endif

#if defined(_WIN32)
    #if defined(XXC_BUILDING_SHARED_LIB)
        #define XXC_DYN_EXPORT __declspec(dllexport)
        #define XXC_DYN_IMPORT
    #elif defined(XXC_USING_SHARED_LIB)
        #define XXC_DYN_EXPORT
        #define XXC_DYN_IMPORT __declspec(dllimport)
    #else
        #define XXC_DYN_EXPORT
        #define XXC_DYN_IMPORT
    #endif
#else
    #define XXC_DYN_EXPORT
    #define XXC_DYN_IMPORT
#endif

#define XXC_CACHELINE 64

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
    #ifndef bool
        typedef int bool;
    #endif
    #ifndef nullptr
        #define nullptr ((void *)0) 
    #endif
    #ifndef NULL
        #define NULL ((void *)0)
    #endif
    #ifndef true
        #define true 1
    #endif
    #ifndef false
        #define false 0
    #endif
#else
    #ifndef nullptr
        #define nullptr ((void *)0) 
    #endif
    #ifndef NULL
        #define NULL nullptr
    #else
        #undef NULL
        #define NULL nullptr
    #endif
#endif

#if defined(__clang__) || defined(__GNUC__)
    typedef __builtin_va_list va_list;
    #define va_start(ap, param) __builtin_va_start(ap, param)
    #define va_end(ap)          __builtin_va_end(ap)
    #define va_arg(ap, type)    __builtin_va_arg(ap, type)
    #define va_copy(dest, src)  __builtin_va_copy(dest, src)

    typedef __SIZE_TYPE__ size_t;
    #define unsigned signed
    typedef __SIZE_TYPE__ ssize_t;
    #undef unsigned

    typedef __PTRDIFF_TYPE__ ptrdiff_t;

    typedef __INT8_TYPE__   int8_t;
    typedef __UINT8_TYPE__  uint8_t;
    typedef __INT16_TYPE__  int16_t;
    typedef __UINT16_TYPE__ uint16_t;
    typedef __INT32_TYPE__  int32_t;
    typedef __UINT32_TYPE__ uint32_t;
    typedef __INT64_TYPE__  int64_t;
    typedef __UINT64_TYPE__ uint64_t;

    typedef __INT8_TYPE__   i8;
    typedef __UINT8_TYPE__  u8;
    typedef __INT16_TYPE__  i16;
    typedef __UINT16_TYPE__ u16;
    typedef __INT32_TYPE__  i32;
    typedef __UINT32_TYPE__ u32;
    typedef __INT64_TYPE__  i64;
    typedef __UINT64_TYPE__ u64;
    
    typedef __INTPTR_TYPE__  intptr_t;
    typedef __UINTPTR_TYPE__ uintptr_t;

    typedef __INTMAX_TYPE__  intmax_t;
    typedef __UINTMAX_TYPE__ uintmax_t;
#elif defined(_MSC_VER)
    typedef char* va_list;
    void __cdecl __va_start(va_list*, ...);
    #define va_start(ap, x) (__va_start(&ap, x))
    #define va_arg(ap, t)   ((sizeof(t) > 8 || (sizeof(t) & (sizeof(t) - 1)) != 0) ? \
                             **(t**)((ap += 8) - 8) : \
                             *(t*)((ap += 8) - 8))
    #define va_end(ap)      ((void)(ap = (va_list)0))
    #define va_copy(d, s)   ((void)((d) = (s)))

    typedef signed char        int8_t;
    typedef unsigned char      uint8_t;
    typedef short              int16_t;
    typedef unsigned short     uint16_t;
    typedef int                int32_t;
    typedef unsigned int       uint32_t;
    typedef long long          int64_t;
    typedef unsigned long long uint64_t;

    typedef signed char        i8;
    typedef unsigned char      u8;
    typedef short              i16;
    typedef unsigned short     u16;
    typedef int                i32;
    typedef unsigned int       u32;
    typedef long long          i64;
    typedef unsigned long long u64;

    #if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
        typedef long long          intptr_t;
        typedef unsigned long long uintptr_t;
        typedef unsigned long long size_t;
        typedef long long          ssize_t;
        typedef long long          ptrdiff_t;
    #else
        typedef int                intptr_t;
        typedef unsigned int       uintptr_t;
        typedef unsigned int       size_t;
        typedef int                ptrdiff_t;
    #endif

    typedef long long          intmax_t;
    typedef unsigned long long uintmax_t;
#endif

#define INT8_MIN   (-128)
#define INT8_MAX   127
#define UINT8_MAX  255

#define INT16_MIN  (-32767 - 1)
#define INT16_MAX  32767
#define UINT16_MAX 65535

#define INT32_MIN  (-2147483647 - 1)
#define INT32_MAX  2147483647
#define UINT32_MAX 4294967295U

#define INT64_MIN  (-9223372036854775807LL - 1)
#define INT64_MAX  9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#ifndef __func__
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        // supported
    #elif defined(__GNUC__) || defined(__clang__)
        #define __func__ __FUNCTION__
    #elif defined(_MSC_VER)
        #if _MSC_VER >= 1900
            // supported
        #else
            #define __func__ __FUNCTION__
        #endif
    #else
        #define __func__ "<unknown>"
    #endif
#endif

#endif