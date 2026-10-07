// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <cond.h>
#include <thread.h>
#include <crt_lock.h>

static u32 mutex_release_for_wait(mutex_t *mtx)
{
    u32 saved = mtx->count;
    mtx->owner_tid = 0;
    mtx->count = 0;
    return saved;
}

static void mutex_reacquire_after_wait(mutex_t *mtx, u32 saved_count)
{
    mtx->owner_tid = (long)thread_current_id();
    mtx->count = saved_count;
}

#if defined(XXC_RAWSYS)
#include <xxc_sys.h>

int cond_init(cond_t *cv)
{
    if (!cv) return COND_ERROR;
    atomic_u32_init(&cv->seq, 0);
    return COND_SUCCESS;
}

int cond_destroy(cond_t *cv) { (void)cv; return COND_SUCCESS; }

static int cond_wait_impl(cond_t *cv, mutex_t *mtx, const xxc_timespec_t *timeout)
{
    if (!cv || !mtx) return COND_ERROR;
    if ((long)thread_current_id() != mtx->owner_tid) return COND_ERROR;

    u32 seq = atomic_u32_load_explicit(&cv->seq, ATOMIC_ACQUIRE);
    u32 saved = mutex_release_for_wait(mtx);
    __crt_lock_release(&mtx->lock);

    long r = sys_futex((int *)&cv->seq, FUTEX_WAIT_PRIVATE, (int)seq, timeout, NULL, 0);

    __crt_lock_acquire(&mtx->lock);
    mutex_reacquire_after_wait(mtx, saved);

    return (r == -XXC_ETIMEDOUT) ? COND_TIMEDOUT : COND_SUCCESS;
}

int cond_wait(cond_t *cv, mutex_t *mtx)
{
    return cond_wait_impl(cv, mtx, NULL);
}

int cond_timedwait_ms(cond_t *cv, mutex_t *mtx, u32 ms)
{
    xxc_timespec_t ts;
    ts.tv_sec  = (long)(ms / 1000u);
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    return cond_wait_impl(cv, mtx, &ts);
}

void cond_signal(cond_t *cv)
{
    if (!cv) return;
    atomic_u32_fetch_add_explicit(&cv->seq, 1, ATOMIC_RELEASE);
    sys_futex((int *)&cv->seq, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

void cond_broadcast(cond_t *cv)
{
    if (!cv) return;
    atomic_u32_fetch_add_explicit(&cv->seq, 1, ATOMIC_RELEASE);
    sys_futex((int *)&cv->seq, FUTEX_WAKE_PRIVATE, (int)0x7fffffff, NULL, NULL, 0);
}

#elif defined(_WIN32)

int cond_init(cond_t *cv)
{
    if (!cv) return COND_ERROR;
    InitializeConditionVariable(&cv->cv);
    return COND_SUCCESS;
}

int cond_destroy(cond_t *cv) { (void)cv; return COND_SUCCESS; }

static int cond_wait_impl(cond_t *cv, mutex_t *mtx, DWORD timeout_ms)
{
    if (!cv || !mtx) return COND_ERROR;
    if ((long)thread_current_id() != mtx->owner_tid) return COND_ERROR;

    u32 saved = mutex_release_for_wait(mtx);
    BOOL ok = SleepConditionVariableSRW(&cv->cv, &mtx->lock, timeout_ms, 0);
    DWORD err = ok ? 0 : GetLastError();
    mutex_reacquire_after_wait(mtx, saved);

    return (!ok && err == ERROR_TIMEOUT) ? COND_TIMEDOUT : COND_SUCCESS;
}

int cond_wait(cond_t *cv, mutex_t *mtx)
{
    return cond_wait_impl(cv, mtx, INFINITE);
}

int cond_timedwait_ms(cond_t *cv, mutex_t *mtx, u32 ms)
{
    return cond_wait_impl(cv, mtx, (DWORD)ms);
}

void cond_signal(cond_t *cv)
{
    if (cv) WakeConditionVariable(&cv->cv);
}

void cond_broadcast(cond_t *cv)
{
    if (cv) WakeAllConditionVariable(&cv->cv);
}

#endif
