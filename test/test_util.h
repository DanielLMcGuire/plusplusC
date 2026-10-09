// ++C Runtime Library
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <def.h>
#include <cio.h>
#include <str.h>
#include <mem.h>
#include <atexit.h>
#include <alloc.h>
#include "test.h"

#if defined(__linux__)
#include <xxc_sys.h>

#define TU_SIGCHLD 17

typedef struct
{
    int saved;
    int rd;
    int fd;
} capture_t;

static bool capture_begin(capture_t *c, int fd)
{
    int p[2];
    if (syscall(__NR_pipe2, (long)p, 0L) < 0)
        return false;
    c->saved = (int)syscall(__NR_dup, (long)fd);
    c->fd    = fd;
    c->rd    = p[0];
    syscall(__NR_dup3, (long)p[1], (long)fd, 0L);
    sys_close(p[1]);
    return true;
}

static long capture_end(capture_t *c, char *buf, size_t cap)
{
    syscall(__NR_dup3, (long)c->saved, (long)c->fd, 0L);
    sys_close(c->saved);

    size_t total = 0;
    for (;;)
    {
        long n = sys_read(c->rd, buf + total, cap - 1 - total);
        if (n <= 0)
            break;
        total += (size_t)n;
        if (total >= cap - 1)
            break;
    }
    buf[total] = '\0';
    sys_close(c->rd);
    return (long)total;
}

static int run_in_child(int (*fn)(void *), void *arg)
{
    long pid = syscall(__NR_clone, (long)TU_SIGCHLD, 0L, 0L, 0L, 0L);
    if (pid < 0)
        return -1000;
    if (pid == 0)
    {
        int r = fn(arg);
        exit(r);
    }

    int st = 0;
    syscall(__NR_wait4, pid, (long)&st, 0L, 0L);
    if ((st & 0x7f) == 0)
        return (st >> 8) & 0xff;
    return -(st & 0x7f);
}

static void *oom_begin(void)
{
    unsigned long lim[2] = {1, 1};
    syscall(__NR_setrlimit, 9L /* RLIMIT_AS */, (long)lim);

    static const size_t sizes[] = { 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128,
                                    160, 192, 224, 256, 320, 384, 448, 512, 640, 768,
                                    896, 1024, 1536, 2048, 3072, 4096 };
    void *head = nullptr;
    for (int round = 0; round < 2; round++)
    {
        for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
        {
            void *p;
            while ((p = malloc(sizes[i])) != nullptr)
            {
                *(void **)p = head;
                head = p;
            }
        }
    }
    return head;
}

static void oom_end(void *head)
{
    while (head)
    {
        void *next = *(void **)head;
        free(head);
        head = next;
    }
}

static bool oom_free_one(void **head, size_t usable)
{
    void **link = head;
    while (*link)
    {
        void *blk = *link;
        if (malloc_usable_size(blk) == usable)
        {
            *link = *(void **)blk;
            free(blk);
            return true;
        }
        link = (void **)blk;
    }
    return false;
}

static int run_in_child_quiet(int (*fn)(void *), void *arg)
{
    long pid = syscall(__NR_clone, (long)TU_SIGCHLD, 0L, 0L, 0L, 0L);
    if (pid < 0)
        return -1000;
    if (pid == 0)
    {
        long nul = sys_openat(AT_FDCWD, "/dev/null", O_WRONLY, 0);
        if (nul >= 0)
        {
            syscall(__NR_dup3, nul, 1L, 0L);
            syscall(__NR_dup3, nul, 2L, 0L);
        }
        int r = fn(arg);
        exit(r);
    }

    int st = 0;
    syscall(__NR_wait4, pid, (long)&st, 0L, 0L);
    if ((st & 0x7f) == 0)
        return (st >> 8) & 0xff;
    return -(st & 0x7f);
}
#endif /* __linux__ */

#endif /* TEST_UTIL_H */
