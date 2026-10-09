// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <sem.h>

#if defined(XXC_RAWSYS)
#include <xxc_sys.h>

int sem_init(sem_t *sem, u32 initial)
{
    if (sem == nullptr) return SEM_ERROR;
    atomic_i32_init(&sem->count, (i32)initial);
    return SEM_SUCCESS;
}

int sem_destroy(sem_t *sem) { (void)sem; return SEM_SUCCESS; }

static int try_take(sem_t *sem, i32 *cur)
{
    i32 c = atomic_i32_load_explicit(&sem->count, ATOMIC_ACQUIRE);
    for (;;)
    {
        if (c <= 0)
        { 
            *cur = c; 
            return 0; 
        }
        if (atomic_i32_cas_weak_explicit(&sem->count, &c, c - 1, ATOMIC_ACQUIRE, ATOMIC_ACQUIRE))
            return 1;
    }
}

static int sem_wait_impl(sem_t *sem, const xxc_timespec_t *timeout)
{
    if (sem == nullptr) return SEM_ERROR;
    i32 cur;
    for (;;)
    {
        if (try_take(sem, &cur)) return SEM_SUCCESS;
        long r = sys_futex((int *)&sem->count, FUTEX_WAIT_PRIVATE, cur, timeout, nullptr, 0);
        if (r == -XXC_ETIMEDOUT) return SEM_TIMEDOUT;
    }
}

int sem_wait(sem_t *sem)
{
    return sem_wait_impl(sem, nullptr);
}

int sem_trywait(sem_t *sem)
{
    if (sem == nullptr) return SEM_ERROR;
    i32 cur;
    return try_take(sem, &cur) ? SEM_SUCCESS : SEM_BUSY;
}

int sem_timedwait_ms(sem_t *sem, u32 ms)
{
    xxc_timespec_t ts;
    ts.tv_sec  = (long)(ms / 1000u);
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    return sem_wait_impl(sem, &ts);
}

int sem_post(sem_t *sem)
{
    if (sem == nullptr) return SEM_ERROR;
    atomic_i32_fetch_add_explicit(&sem->count, 1, ATOMIC_RELEASE);
    sys_futex((int *)&sem->count, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
    return SEM_SUCCESS;
}

#elif defined(_WIN32)

int sem_init(sem_t *sem, u32 initial)
{
    if (sem == nullptr) return SEM_ERROR;
    sem->handle = CreateSemaphoreW(nullptr, (LONG)initial, 0x7fffffffL, nullptr);
    return sem->handle ? SEM_SUCCESS : SEM_ERROR;
}

int sem_destroy(sem_t *sem)
{
    if (sem == nullptr || sem->handle == nullptr) return SEM_ERROR;
    CloseHandle(sem->handle);
    sem->handle = nullptr;
    return SEM_SUCCESS;
}

static int sem_wait_impl(sem_t *sem, DWORD timeout_ms)
{
    if (sem == nullptr || sem->handle == nullptr) return SEM_ERROR;
    DWORD r = WaitForSingleObject(sem->handle, timeout_ms);
    if (r == WAIT_OBJECT_0) return SEM_SUCCESS;
    if (r == WAIT_TIMEOUT)  return SEM_TIMEDOUT;
    return SEM_ERROR;
}

int sem_wait(sem_t *sem)             { return sem_wait_impl(sem, INFINITE); }
int sem_trywait(sem_t *sem)          { int r = sem_wait_impl(sem, 0); return r == SEM_TIMEDOUT ? SEM_BUSY : r; }
int sem_timedwait_ms(sem_t *sem, u32 ms) { return sem_wait_impl(sem, (DWORD)ms); }

int sem_post(sem_t *sem)
{
    if (sem == nullptr || sem->handle == nullptr) return SEM_ERROR;
    return ReleaseSemaphore(sem->handle, 1, nullptr) ? SEM_SUCCESS : SEM_ERROR;
}

#endif
