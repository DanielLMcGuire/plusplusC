// ++C C Runtime Library (libminicrt) | Platform (Linux/FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include "xxc_sys.h"
#include <atomic.h>

#define XXC_TLS_KEYS_MAX 64

enum {
    XXC_THREAD_JOINABLE = 0,
    XXC_THREAD_DETACHED = 1,
    XXC_THREAD_EXITED   = 2
};

#if defined(__i386__)
typedef struct {
    u32 entry_number;
    u32 base_addr;
    u32 limit;
    u32 flags;
} xxc_user_desc_t;
#endif

struct xxc_thread {
    struct xxc_thread *self;
    atomic_i32_t       tid;
    atomic_i32_t       state;
    void *(*start)(void *);
    void              *arg;
    void              *retval;
    void              *map_base;
    size_t             map_size;
#if defined(__FreeBSD__)
    volatile long      ktid;
    volatile long      exit_state;
#endif
    void              *tls_values[XXC_TLS_KEYS_MAX];
    u32                tls_gens[XXC_TLS_KEYS_MAX];
#if defined(__i386__)
    xxc_user_desc_t    user_desc;
#endif
};

extern __thread struct xxc_thread *__xxc_self;
extern struct xxc_thread           __xxc_main_thread;

#if defined(__i386__)
void __xxc_fill_user_desc(struct xxc_thread *t, void *tp);
#endif

size_t __xxc_tls_area_size(void);

char *__xxc_tls_setup(char *region_end, void **tp_out);

#if defined(__linux__)
int  __xxc_clone(int (*fn)(void *), void *stack_top, int flags, void *arg,
                 int *ptid, void *tls, int *ctid);
#endif
void __xxc_unmapself(void *base, size_t size) XXC_NORETURN;

void __xxc_heap_thread_exit(void);

#endif /* SYS_THREAD_H */
