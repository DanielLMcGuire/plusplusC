// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <cio.h>
#include <dstr.h>
#include <alloc.h>
#include <parr.h>
#include <str.h>
#include <def.h>
#include <crt_lock.h>

#ifdef __UEFI__
#include <sys_uefi.h>
#elif defined(_WIN32)
#include <windows.h>
static HANDLE g_err = INVALID_HANDLE_VALUE;
static HANDLE g_out = INVALID_HANDLE_VALUE;
#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>
#endif

#define PRINTF_STACK_BUF_SIZE 1024

static crt_lock_t g_stdout_lock = CRT_LOCK_INIT;
static crt_lock_t g_stderr_lock = CRT_LOCK_INIT;

static inline crt_lock_t *sio_lock_for(unsigned int stream)
{
    return (stream == SIOOUT) ? &g_stdout_lock : &g_stderr_lock;
}

void sio_init(unsigned int out, unsigned int err)
{
#ifdef _WIN32
    g_err = GetStdHandle(err);
    g_out = GetStdHandle(out);
#endif
    (void)out; (void)err;
}

static int raw_print(const char *str, unsigned int stream)
{
    if (str == NULL) return -1;

#ifdef __UEFI__
    __uefi_write(str, strlen(str), stream);
#elif defined(_WIN32)
    HANDLE handle = (stream == SIOOUT) ? g_out :
                    ((stream == SIOERR) ? g_err : INVALID_HANDLE_VALUE);

    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return -1;

    DWORD length = (DWORD)strlen(str);
    if (length == 0) return 1;

    DWORD written = 0;
    DWORD file_type = GetFileType(handle);
    DWORD console_mode = 0;

    if (file_type == FILE_TYPE_CHAR && GetConsoleMode(handle, &console_mode))
    {
        if (!WriteConsoleA(handle, str, length, &written, NULL)) return -1;
    }
    else
    {
        if (!WriteFile(handle, str, length, &written, NULL)) return -1;
    }

#elif defined(XXC_RAWSYS)
    int fd = (stream == SIOOUT) ? 1 : 2;

    if (sys_write(fd, str, strlen(str)) < 0) return -1;
#endif

    return 1;
}

static int raw_putchar(int c, unsigned int stream)
{
#ifdef __UEFI__
    char ch = (char)c;
    __uefi_write(&ch, 1, stream);
#elif defined(_WIN32)
    HANDLE handle = (stream == SIOOUT) ? g_out :
                    ((stream == SIOERR) ? g_err : INVALID_HANDLE_VALUE);

    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return -1;

    char ch = (char)c;
    DWORD written = 0;
    DWORD file_type = GetFileType(handle);
    DWORD console_mode = 0;

    if (file_type == FILE_TYPE_CHAR && GetConsoleMode(handle, &console_mode))
    {
        if (!WriteConsoleA(handle, &ch, 1, &written, NULL))
            return -1;
    }
    else
    {
        if (!WriteFile(handle, &ch, 1, &written, NULL)) 
            return -1;
    }

#elif defined(XXC_RAWSYS)
    char ch = (char)c;
    int fd = (stream == SIOOUT) ? 1 : 2;

    if (sys_write(fd, &ch, 1) < 0) return -1;
#endif

    return c;
}

int print(const char *str, unsigned int stream)
{
    crt_lock_t *lock = sio_lock_for(stream);

    __crt_lock_acquire(lock);
    int r = raw_print(str, stream);
    __crt_lock_release(lock);

    return r;
}

int putchar(int c, unsigned int stream)
{
    crt_lock_t *lock = sio_lock_for(stream);

    __crt_lock_acquire(lock);
    int r = raw_putchar(c, stream);
    __crt_lock_release(lock);

    return r;
}

int vprintf(const char *fmt, va_list args, unsigned int stream)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(NULL, 0, fmt, args);
    if (len < 0)
    {
        va_end(args_copy);
        return len;
    }

    size_t size = (size_t)len + 1;
    char *buf = NULL;
    bool owns_buf = false;

    if (size < PRINTF_STACK_BUF_SIZE) 
    {
        buf = alloca(size);
    } 
    else 
    {
        buf = (char *)malloc(size);
        owns_buf = true;
    }

    if (buf) 
    {
        vsnprintf(buf, size, fmt, args_copy);

        print(buf, stream);
    }

    if (owns_buf) 
        free(buf);

    va_end(args_copy);
    return len;
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args, SIOOUT);
    va_end(args);
    return ret;
}

int puts(const char *str, unsigned int stream)
{
    crt_lock_t *lock = sio_lock_for(stream);

    __crt_lock_acquire(lock);

    int r = raw_print(str, stream);

    if (r == 1)
        r = raw_putchar('\n', stream);

    __crt_lock_release(lock);

    return r;
}

int puts_ds(dstr_t *s, unsigned int stream)
{
    return puts(s->data, stream);
}

int print_ds(dstr_t *s, unsigned int stream)
{
    return print(s->data, stream);
}

void print_parr_c_string(const parr_t *csArr, unsigned int stream)
{
    if (!csArr) return;

    dstr_t out = dstr_new("[");

    for (size_t i = 0; i < csArr->len; i++)
    {
        const char *item = (const char*)csArr->data[i];

        dstr_append_char(&out, '"');
        if (item)
            dstr_append(&out, item);
        dstr_append_char(&out, '"');

        if (i + 1 < csArr->len)
            dstr_append(&out, ", ");
    }

    dstr_append_char(&out, ']');

    puts(out.data, stream);
    dstr_free(&out);
}

void print_parr_dstr(const parr_t *dsArr, unsigned int stream)
{
    if (!dsArr) return;

    dstr_t out = dstr_new("[");

    for (size_t i = 0; i < dsArr->len; i++)
    {
        const dstr_t *item = (const dstr_t*)dsArr->data[i];

        dstr_append_char(&out, '"');
        
        if (item && item->data)
            dstr_append(&out, item->data);
        
        dstr_append_char(&out, '"');

        if (i + 1 < dsArr->len) 
            dstr_append(&out, ", ");
    }

    dstr_append_char(&out, ']');

    puts(out.data, stream);
    dstr_free(&out);
}