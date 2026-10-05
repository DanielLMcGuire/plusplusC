// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License
#ifndef CRT_LOCK_H
#define CRT_LOCK_H
#include <def.h>

#if defined(_WIN32)
#include <windows.h>
typedef SRWLOCK crt_lock_t;
#define CRT_LOCK_INIT SRWLOCK_INIT

static inline void __crt_lock_init(crt_lock_t *lock)
{
    InitializeSRWLock(lock);
}

static inline void __crt_lock_acquire(crt_lock_t *lock)
{
    AcquireSRWLockExclusive(lock);
}

static inline bool __crt_lock_tryacquire(crt_lock_t *lock)
{
    return TryAcquireSRWLockExclusive(lock) != 0;
}

static inline void __crt_lock_release(crt_lock_t *lock)
{
    ReleaseSRWLockExclusive(lock);
}

#elif defined(__linux__)
#include <sys_linux.h>
typedef struct {
    int state;
} crt_lock_t;
#define CRT_LOCK_INIT { 0 }

static inline void __crt_lock_init(crt_lock_t *lock)
{
    lock->state = 0;
}

static inline void __crt_lock_acquire(crt_lock_t *lock)
{
    int expected = 0;
    if (__atomic_compare_exchange_n(&lock->state, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return;
    for (int i = 0; i < 40; i++)
    {
        expected = 0;
        if (lock->state == 0 && __atomic_compare_exchange_n(&lock->state, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
            return;
#if defined(__x86_64__) || defined(__i386__)
        __asm__ volatile("pause");
#elif defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("yield");
#endif
    }
    int c = __atomic_exchange_n(&lock->state, 2, __ATOMIC_ACQUIRE);
    while (c != 0)
    {
        sys_futex(&lock->state, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
        c = __atomic_exchange_n(&lock->state, 2, __ATOMIC_ACQUIRE);
    }
}

static inline bool __crt_lock_tryacquire(crt_lock_t *lock)
{
    int expected = 0;
    return __atomic_compare_exchange_n(&lock->state, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

static inline void __crt_lock_release(crt_lock_t *lock)
{
    if (__atomic_fetch_sub(&lock->state, 1, __ATOMIC_RELEASE) != 1)
    {
        __atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
        sys_futex(&lock->state, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
    }
}

#else
typedef int crt_lock_t;
#define CRT_LOCK_INIT 0
static inline void __crt_lock_init(crt_lock_t *lock)          { (void)lock; }
static inline void __crt_lock_acquire(crt_lock_t *lock)       { (void)lock; }
static inline bool __crt_lock_tryacquire(crt_lock_t *lock)     { (void)lock; return true; }
static inline void __crt_lock_release(crt_lock_t *lock)       { (void)lock; }
#endif

#endif /* CRT_LOCK_H */