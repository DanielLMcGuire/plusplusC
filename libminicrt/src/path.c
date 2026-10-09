// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <path.h>
#include <alloc.h>
#include <str.h>
#include <mem.h>

#if defined(__UEFI__)
#include <sys_uefi.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>
#endif

#define XXC_ERANGE      34
#define CWD_START_CAP   256
#define CWD_MAX_CAP     (1u << 20)

static long cwd_raw(char *buf, size_t size)
{
#if defined(__UEFI__)
    if (size < 2) return 0;
    buf[0] = '/';
    buf[1] = '\0';
    return 2;
#elif defined(_WIN32)
    DWORD n = GetCurrentDirectoryA((DWORD)size, buf);
    if (n == 0) return -1;
    if ((size_t)n >= size) return 0;
    return (long)n + 1;
#elif defined(XXC_RAWSYS)
    long r = sys_getcwd(buf, size);
    if (r == -XXC_ERANGE) return 0;
    if (r <= 0) return -1;
    if (buf[0] != '/') return -1;
    return r;
#else
    (void)buf; (void)size;
    return -1;
#endif
}

char *getcwd(char *buf, size_t size)
{
    if (buf)
    {
        if (size == 0) return NULL;
        return cwd_raw(buf, size) > 0 ? buf : NULL;
    }

    size_t cap = size ? size : CWD_START_CAP;
    for (;;)
    {
        char *p = (char *)malloc(cap);
        if (!p) return NULL;

        long r = cwd_raw(p, cap);
        if (r > 0) return p;

        free(p);
        if (r < 0 || size != 0 || cap >= CWD_MAX_CAP) return NULL;
        cap *= 2;
    }
}

bool path_getcwd(dstr_t *out)
{
    if (!out) return false;

    char *cwd = getcwd(NULL, 0);
    *out = dstr_new(cwd ? cwd : "");
    if (cwd) free(cwd);
    return cwd != NULL;
}
