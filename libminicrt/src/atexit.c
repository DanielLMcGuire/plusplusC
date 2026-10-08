// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License
#include <atexit.h>
#include <crt_lock.h>

#ifdef __UEFI__
#include <sys_uefi.h>

void _exit(int status)
{
    __uefi_halt(status);
}

#elif defined(_WIN32)
#include <windows.h>

void _exit(int status)
{
    TerminateProcess(GetCurrentProcess(), (UINT)status);
    __NORETURN__ // GCOV_EXCL_LINE
}

#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>

void _exit(int status)
{
    sys_exit(status);
    __NORETURN__ // GCOV_EXCL_LINE
}
#else

void _exit(int status)
{
    (void)status;
    __NORETURN__ // GCOV_EXCL_LINE
}
#endif

static atexit_func_t exit_handlers[MAX_ATEXIT_FUNCS];
static int handler_count = 0;
static crt_lock_t g_atexit_lock = CRT_LOCK_INIT;
static bool g_exiting = false;

int atexit(atexit_func_t func)
{
    if (func == NULL)
        return -1;
    __crt_lock_acquire(&g_atexit_lock);
    if (handler_count >= MAX_ATEXIT_FUNCS)
    {
        __crt_lock_release(&g_atexit_lock);
        return -1;
    }
    exit_handlers[handler_count++] = func;
    __crt_lock_release(&g_atexit_lock);
    return 0;
}

XXC_NORETURN void exit(int status)
{
    __crt_lock_acquire(&g_atexit_lock);
    /* GCOV_EXCL_START */
    if (g_exiting)
    {
        __crt_lock_release(&g_atexit_lock);
        __NORETURN__
    }
    g_exiting = true;
    __crt_lock_release(&g_atexit_lock);

    for (;;)
    {
        atexit_func_t func;
        __crt_lock_acquire(&g_atexit_lock);
        if (handler_count <= 0)
        {
            __crt_lock_release(&g_atexit_lock);
            break;
        }
        func = exit_handlers[--handler_count];
        __crt_lock_release(&g_atexit_lock);
        if (func != NULL)
            func();
    }

    _exit(status);
    __NORETURN__
    /* GCOV_EXCL_STOP */
}