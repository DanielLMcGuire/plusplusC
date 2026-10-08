// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <thread.h>
#include <atomic.h>
#include <alloc.h>
#include <atexit.h>
#include <crt_lock.h>

static struct {
    void (*dtor)(void *);
    atomic_u32_t gen;
    bool used;
} g_keys[TLS_KEYS_MAX];

static crt_lock_t g_key_lock = CRT_LOCK_INIT;

int tls_key_create(tls_key_t *key, void (*destructor)(void *))
{
    if (!key)
        return THREAD_INVAL;

    int rc = THREAD_NOMEM;
    __crt_lock_acquire(&g_key_lock);
    for (u32 i = 0; i < TLS_KEYS_MAX; i++)
    {
        if (g_keys[i].used) continue;
        g_keys[i].used = true;
        g_keys[i].dtor = destructor;
        atomic_u32_fetch_add(&g_keys[i].gen, 1);
        *key = i;
        rc = THREAD_SUCCESS;
        break;
    }
    __crt_lock_release(&g_key_lock);
    return rc;
}

int tls_key_delete(tls_key_t key)
{
    if (key >= TLS_KEYS_MAX)
        return THREAD_INVAL;

    int rc = THREAD_INVAL;
    __crt_lock_acquire(&g_key_lock);
    if (g_keys[key].used)
    {
        g_keys[key].used = false;
        g_keys[key].dtor = NULL;
        atomic_u32_fetch_add(&g_keys[key].gen, 1);
        rc = THREAD_SUCCESS;
    }
    __crt_lock_release(&g_key_lock);
    return rc;
}

#if defined(XXC_RAWSYS)

#include "sys_thread.h"

#define XXC_THREAD_GUARD  ((size_t)65536)
#define XXC_THREAD_MINSTK ((size_t)16384)

#if defined(__linux__)
static int g_dead_tid;
#endif

static inline size_t align_up_sz(size_t v, size_t a)
{ 
    return (v + a - 1) & ~(a - 1); 
}

void *tls_get(tls_key_t key)
{
    if (key >= TLS_KEYS_MAX) return NULL;
    struct xxc_thread *t = __xxc_self;
    if (t->tls_gens[key] != atomic_u32_load_explicit(&g_keys[key].gen, ATOMIC_ACQUIRE))
        return NULL;
    return t->tls_values[key];
}

int tls_set(tls_key_t key, void *value)
{
    if (key >= TLS_KEYS_MAX || !g_keys[key].used) return THREAD_INVAL;
    struct xxc_thread *t = __xxc_self;
    t->tls_gens[key] = atomic_u32_load_explicit(&g_keys[key].gen, ATOMIC_ACQUIRE);
    t->tls_values[key] = value;
    return THREAD_SUCCESS;
}

static void run_tls_destructors(struct xxc_thread *t)
{
    for (int round = 0; round < 4; round++)
    {
        bool ran = false;
        for (u32 k = 0; k < TLS_KEYS_MAX; k++)
        {
            void (*dtor)(void *) = NULL;
            __crt_lock_acquire(&g_key_lock);
            if (g_keys[k].used && t->tls_gens[k] == atomic_u32_load(&g_keys[k].gen))
                dtor = g_keys[k].dtor;
            __crt_lock_release(&g_key_lock);

            void *v = t->tls_values[k];
            if (dtor && v)
            {
                t->tls_values[k] = NULL;
                dtor(v);
                ran = true;
            }
        }
        if (!ran) break;
    }
}

#if defined(__linux__)

static XXC_NORETURN void thread_finish(struct xxc_thread *t, void *retval)
{
    t->retval = retval;
    run_tls_destructors(t);
    __xxc_heap_thread_exit();

    i32 expect = XXC_THREAD_JOINABLE;
    if (atomic_i32_cas(&t->state, &expect, XXC_THREAD_EXITED))
        sys_exit_thread(0);

    u64 all = ~(u64)0;
    (void)sys_rt_sigprocmask(0, &all, NULL, 8);
    (void)sys_set_tid_address(&g_dead_tid);
    __xxc_unmapself(t->map_base, t->map_size);
}

static int thread_entry(void *p)
{
    struct xxc_thread *t = (struct xxc_thread *)p;
    __xxc_self = t;
    thread_finish(t, t->start(t->arg));
}

int thread_create(thread_t *out, thread_fn_t fn, void *arg, const thread_attr_t *attr)
{
    if (!out || !fn)
        return THREAD_INVAL;

    size_t stack = (attr && attr->stack_size) ? attr->stack_size : THREAD_DEFAULT_STACK_SIZE;
    if (stack < XXC_THREAD_MINSTK) stack = XXC_THREAD_MINSTK;
    bool detached = attr && attr->detached;

    size_t tls_area = align_up_sz(__xxc_tls_area_size(), 64);
    size_t desc_sz  = align_up_sz(sizeof(struct xxc_thread), 64);
    size_t total    = align_up_sz(XXC_THREAD_GUARD + stack + tls_area + desc_sz, 65536);

    char *base = (char *)sys_mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (!base)
        return THREAD_NOMEM;
    (void)sys_mprotect(base, XXC_THREAD_GUARD, PROT_NONE);

    struct xxc_thread *t = (struct xxc_thread *)(base + total - desc_sz);
    t->self     = t;
    t->start    = fn;
    t->arg      = arg;
    t->map_base = base;
    t->map_size = total;
    atomic_i32_init(&t->state, detached ? XXC_THREAD_DETACHED : XXC_THREAD_JOINABLE);

    void *tp;
    char *lowest = __xxc_tls_setup((char *)t, &tp);
    char *sp = (char *)((uintptr_t)lowest & ~(uintptr_t)15);

    void *tls_arg = tp;
#if defined(__i386__)
    __xxc_fill_user_desc(t, tp);
    tls_arg = &t->user_desc;
#endif

    int flags = XXC_CLONE_VM | XXC_CLONE_FS | XXC_CLONE_FILES | XXC_CLONE_SIGHAND |
                XXC_CLONE_THREAD | XXC_CLONE_SYSVSEM | XXC_CLONE_SETTLS |
                XXC_CLONE_PARENT_SETTID | XXC_CLONE_CHILD_CLEARTID;

    *out = detached ? NULL : t;
    long r = __xxc_clone(thread_entry, sp, flags, t, (int *)&t->tid, tls_arg, (int *)&t->tid);
    if (r < 0)
    {
        *out = NULL;
        sys_munmap(base, total);
        return (r == -XXC_ENOMEM || r == -XXC_EAGAIN) ? THREAD_NOMEM : THREAD_ERROR;
    }
    return THREAD_SUCCESS;
}

static void thread_reap(struct xxc_thread *t, void **retval)
{
    for (;;)
    {
        i32 tid = atomic_i32_load_explicit(&t->tid, ATOMIC_ACQUIRE);
        if (tid == 0) break;
        sys_futex((int *)&t->tid, 0, tid, NULL, NULL, 0);
    }
    if (retval) *retval = t->retval;
    void *base = t->map_base;
    size_t size = t->map_size;
    if (base) sys_munmap(base, size);
}

#elif defined(__FreeBSD__)

static XXC_NORETURN void thread_finish(struct xxc_thread *t, void *retval)
{
    t->retval = retval;
    run_tls_destructors(t);
    __xxc_heap_thread_exit();

    i32 expect = XXC_THREAD_JOINABLE;
    if (atomic_i32_cas(&t->state, &expect, XXC_THREAD_EXITED))
        sys_thr_exit((long *)&t->exit_state);

    (void)sys_sigprocmask_block_all();
    __xxc_unmapself(t->map_base, t->map_size);
}

static XXC_NORETURN void thread_entry(void *p)
{
    struct xxc_thread *t = (struct xxc_thread *)p;
    __xxc_self = t;
    atomic_i32_store(&t->tid, (i32)t->ktid);
    thread_finish(t, t->start(t->arg));
}

int thread_create(thread_t *out, thread_fn_t fn, void *arg, const thread_attr_t *attr)
{
    if (!out || !fn)
        return THREAD_INVAL;

    size_t stack = (attr && attr->stack_size) ? attr->stack_size : THREAD_DEFAULT_STACK_SIZE;
    if (stack < XXC_THREAD_MINSTK) stack = XXC_THREAD_MINSTK;
    bool detached = attr && attr->detached;

    size_t tls_area = align_up_sz(__xxc_tls_area_size(), 64);
    size_t desc_sz  = align_up_sz(sizeof(struct xxc_thread), 64);
    size_t total    = align_up_sz(XXC_THREAD_GUARD + stack + tls_area + desc_sz, 65536);

    char *base = (char *)sys_mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (!base)
        return THREAD_NOMEM;
    (void)sys_mprotect(base, XXC_THREAD_GUARD, PROT_NONE);

    struct xxc_thread *t = (struct xxc_thread *)(base + total - desc_sz);
    t->self     = t;
    t->start    = fn;
    t->arg      = arg;
    t->map_base = base;
    t->map_size = total;
    t->ktid       = 0;
    t->exit_state = 0;
    atomic_i32_init(&t->state, detached ? XXC_THREAD_DETACHED : XXC_THREAD_JOINABLE);

    void *tp;
    char *lowest = __xxc_tls_setup((char *)t, &tp);
    char *sp = (char *)((uintptr_t)lowest & ~(uintptr_t)15);

    xxc_thr_param_t pr = {0};
    pr.start_func = thread_entry;
    pr.arg        = t;
    pr.stack_base = base + XXC_THREAD_GUARD;
    pr.stack_size = (size_t)(sp - pr.stack_base);
    pr.tls_base   = (char *)tp;
    pr.child_tid  = (long *)&t->ktid;
    pr.parent_tid = (long *)&t->ktid;

    *out = detached ? NULL : t;
    long r = sys_thr_new(&pr);
    if (r < 0)
    {
        *out = NULL;
        sys_munmap(base, total);
        return (r == -XXC_ENOMEM || r == -XXC_EAGAIN) ? THREAD_NOMEM : THREAD_ERROR;
    }

    if (!detached)
        atomic_i32_store(&t->tid, (i32)t->ktid);
    return THREAD_SUCCESS;
}

static void thread_reap(struct xxc_thread *t, void **retval)
{
    for (;;)
    {
        i32 st = (i32)__atomic_load_n((volatile i32 *)&t->exit_state, __ATOMIC_ACQUIRE);
        if (st == 1) break;
        sys_futex((int *)&t->exit_state, 0, st, NULL, NULL, 0);
    }
    if (retval) *retval = t->retval;
    void *base = t->map_base;
    size_t size = t->map_size;
    if (base) sys_munmap(base, size);
}

#endif /* __linux__ / __FreeBSD__ */

int thread_join(thread_t t, void **retval)
{
    if (!t) return THREAD_INVAL;
    if (t == __xxc_self) return THREAD_DEADLK;
    if (atomic_i32_load(&t->state) == XXC_THREAD_DETACHED) return THREAD_INVAL;
    thread_reap(t, retval);
    return THREAD_SUCCESS;
}

int thread_detach(thread_t t)
{
    if (!t || t == &__xxc_main_thread) return THREAD_INVAL;

    i32 expect = XXC_THREAD_JOINABLE;
    if (atomic_i32_cas(&t->state, &expect, XXC_THREAD_DETACHED))
        return THREAD_SUCCESS;
    if (expect == XXC_THREAD_EXITED)
    {
        thread_reap(t, NULL);
        return THREAD_SUCCESS;
    }
    return THREAD_INVAL;
}

thread_t thread_self(void) { return __xxc_self; }
bool thread_equal(thread_t a, thread_t b) { return a == b; }
u32 thread_current_id(void) { return (u32)atomic_i32_load_explicit(&__xxc_self->tid, ATOMIC_RELAXED); }
u32 thread_id(thread_t t) { return t ? (u32)atomic_i32_load_explicit(&t->tid, ATOMIC_RELAXED) : 0; }

void thread_exit(void *retval)
{
    struct xxc_thread *t = __xxc_self;
    if (t == &__xxc_main_thread)
        exit(0);
    thread_finish(t, retval);
}

void thread_yield(void)
{
    (void)sys_sched_yield();
}

void thread_sleep_ms(u32 ms)
{
    xxc_timespec_t req, rem;
    req.tv_sec  = (long)(ms / 1000u);
    req.tv_nsec = (long)(ms % 1000u) * 1000000L;
    while (sys_nanosleep(&req, &rem) == -XXC_EINTR)
        req = rem;
}

#elif defined(_WIN32)

#include <windows.h>

struct xxc_thread {
    HANDLE       handle;
    DWORD        tid;
    thread_fn_t  start;
    void        *arg;
    void        *retval;
    atomic_i32_t state;
    void        *tls_values[TLS_KEYS_MAX];
    u32          tls_gens[TLS_KEYS_MAX];
};

enum { WT_JOINABLE = 0, WT_DETACHED = 1, WT_EXITED = 2 };

static DWORD self_index(void)
{
    static volatile LONG idx = -1;
    LONG v = idx;
    if (v == -1)
    {
        DWORD n = TlsAlloc();
        LONG prev = InterlockedCompareExchange(&idx, (LONG)n, -1);
        if (prev != -1)
        { 
            TlsFree(n); 
            n = (DWORD)prev; 
        }
        v = (LONG)n;
    }
    return (DWORD)v;
}

thread_t thread_self(void)
{
    DWORD idx = self_index();
    struct xxc_thread *t = (struct xxc_thread *)TlsGetValue(idx);
    if (!t)
    {
        t = (struct xxc_thread *)calloc(1, sizeof(*t));
        if (!t) return NULL;
        t->tid = GetCurrentThreadId();
        atomic_i32_init(&t->state, WT_DETACHED);
        TlsSetValue(idx, t);
    }
    return t;
}

void *tls_get(tls_key_t key)
{
    if (key >= TLS_KEYS_MAX) return NULL;
    struct xxc_thread *t = thread_self();
    if (!t || t->tls_gens[key] != atomic_u32_load_explicit(&g_keys[key].gen, ATOMIC_ACQUIRE))
        return NULL;
    return t->tls_values[key];
}

int tls_set(tls_key_t key, void *value)
{
    if (key >= TLS_KEYS_MAX || !g_keys[key].used) return THREAD_INVAL;
    struct xxc_thread *t = thread_self();
    if (!t) return THREAD_NOMEM;
    t->tls_gens[key] = atomic_u32_load_explicit(&g_keys[key].gen, ATOMIC_ACQUIRE);
    t->tls_values[key] = value;
    return THREAD_SUCCESS;
}

static void run_tls_destructors(struct xxc_thread *t)
{
    for (int round = 0; round < 4; round++)
    {
        bool ran = false;
        for (u32 k = 0; k < TLS_KEYS_MAX; k++)
        {
            void (*dtor)(void *) = NULL;
            __crt_lock_acquire(&g_key_lock);
            if (g_keys[k].used && t->tls_gens[k] == atomic_u32_load(&g_keys[k].gen))
                dtor = g_keys[k].dtor;
            __crt_lock_release(&g_key_lock);

            void *v = t->tls_values[k];
            if (dtor && v)
            {
                t->tls_values[k] = NULL;
                dtor(v);
                ran = true;
            }
        }
        if (!ran) break;
    }
}

extern void __xxc_emutls_thread_exit(void);

static void thread_finish(struct xxc_thread *t, void *retval)
{
    t->retval = retval;
    run_tls_destructors(t);
    __xxc_emutls_thread_exit();

    i32 expect = WT_JOINABLE;
    if (!atomic_i32_cas(&t->state, &expect, WT_EXITED))
        free(t);
}

static DWORD WINAPI thread_entry(LPVOID p)
{
    struct xxc_thread *t = (struct xxc_thread *)p;
    TlsSetValue(self_index(), t);
    thread_finish(t, t->start(t->arg));
    return 0;
}

int thread_create(thread_t *out, thread_fn_t fn, void *arg, const thread_attr_t *attr)
{
    if (!out || !fn) return THREAD_INVAL;

    struct xxc_thread *t = (struct xxc_thread *)calloc(1, sizeof(*t));
    if (!t) return THREAD_NOMEM;

    bool detached = attr && attr->detached;
    size_t stack = attr ? attr->stack_size : 0;
    t->start = fn;
    t->arg = arg;
    atomic_i32_init(&t->state, detached ? WT_DETACHED : WT_JOINABLE);

    t->handle = CreateThread(NULL, stack, thread_entry, t,
                             CREATE_SUSPENDED | (stack ? STACK_SIZE_PARAM_IS_A_RESERVATION : 0), &t->tid);
    if (!t->handle)
    {
        free(t);
        return THREAD_ERROR;
    }

    HANDLE h = t->handle;
    *out = detached ? NULL : t;
    if (ResumeThread(h) == (DWORD)-1)
    {
        TerminateThread(h, 0);
        CloseHandle(h);
        free(t);
        *out = NULL;
        return THREAD_ERROR;
    }
    if (detached)
        CloseHandle(h);
    return THREAD_SUCCESS;
}

int thread_join(thread_t t, void **retval)
{
    if (!t) return THREAD_INVAL;
    if (t == thread_self()) return THREAD_DEADLK;
    if (atomic_i32_load(&t->state) == WT_DETACHED) return THREAD_INVAL;

    WaitForSingleObject(t->handle, INFINITE);
    if (retval) *retval = t->retval;
    CloseHandle(t->handle);
    free(t);
    return THREAD_SUCCESS;
}

int thread_detach(thread_t t)
{
    if (!t) return THREAD_INVAL;

    HANDLE h = t->handle;
    i32 expect = WT_JOINABLE;
    if (atomic_i32_cas(&t->state, &expect, WT_DETACHED))
    {
        CloseHandle(h);
        return THREAD_SUCCESS;
    }
    if (expect == WT_EXITED)
    {
        CloseHandle(h);
        free(t);
        return THREAD_SUCCESS;
    }
    return THREAD_INVAL;
}

bool thread_equal(thread_t a, thread_t b) { return a == b; }
u32  thread_current_id(void) { return (u32)GetCurrentThreadId(); }
u32  thread_id(thread_t t) { return t ? (u32)t->tid : 0; }

void thread_exit(void *retval)
{
    struct xxc_thread *t = (struct xxc_thread *)TlsGetValue(self_index());
    if (t)
        thread_finish(t, retval);
    ExitThread(0);
    __NORETURN__ // GCOV_EXCL_LINE
}

void thread_yield(void) { SwitchToThread(); }
void thread_sleep_ms(u32 ms) { Sleep(ms); }

#endif
