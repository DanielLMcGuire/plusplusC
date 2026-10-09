// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <mutex.h>
#include <alloc.h>
#include <crt_lock.h>

#if defined(__GNUC__) || defined(__clang__)
    #define crt_atomic_load_long(ptr)       __atomic_load_n((ptr), __ATOMIC_RELAXED)
    #define crt_atomic_store_long(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_RELAXED)
#elif defined(_MSC_VER)
    #define crt_atomic_load_long(ptr)       (*(const volatile long *)(ptr))
    #define crt_atomic_store_long(ptr, val) (*(volatile long *)(ptr) = (val))
#else
    #define crt_atomic_load_long(ptr)       (*(ptr))
    #define crt_atomic_store_long(ptr, val) (*(ptr) = (val))
#endif

#if defined(_WIN32)
#include <windows.h>
static inline long mutex_current_tid(void)
{
    return (long)GetCurrentThreadId();
}
#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>
static inline long mutex_current_tid(void)
{
    long tid = sys_gettid();
    return (tid > 0) ? tid : 1;
}
#else
static inline long mutex_current_tid(void)
{
    return 1;
}
#endif

int mutex_init(mutex_t *mtx, int type)
{
    if (mtx == nullptr)
        return MUTEX_ERROR;

    if (type != MUTEX_PLAIN && type != MUTEX_RECURSIVE && type != MUTEX_ERRORCHECK)
        return MUTEX_ERROR;

    __crt_lock_init(&mtx->lock);
    mtx->owner_tid = 0;
    mtx->count = 0;
    mtx->type = (u8)type;

    return MUTEX_SUCCESS;
}

int mutex_destroy(mutex_t *mtx)
{
    if (mtx == nullptr)
        return MUTEX_ERROR;

    if (crt_atomic_load_long(&mtx->owner_tid) != 0 || mtx->count != 0)
        return MUTEX_BUSY;

    mtx->owner_tid = 0;
    mtx->count = 0;
    return MUTEX_SUCCESS;
}

mutex_t *mutex_create(int type)
{
    mutex_t *mtx = (mutex_t *)malloc(sizeof(mutex_t));
    if (mtx == nullptr)
        return nullptr;

    if (mutex_init(mtx, type) != MUTEX_SUCCESS)
    {
        free(mtx);
        return nullptr;
    }

    return mtx;
}

void mutex_free(mutex_t *mtx)
{
    if (mtx == nullptr)
        return;

    mutex_destroy(mtx);
    free(mtx);
}

int mutex_lock(mutex_t *mtx)
{
    if (mtx == nullptr)
        return MUTEX_ERROR;

    long tid = mutex_current_tid();

    if (mtx->type == MUTEX_RECURSIVE)
    {
        if (crt_atomic_load_long(&mtx->owner_tid) == tid)
        {
            if (mtx->count == UINT32_MAX)
                return MUTEX_ERROR;

            mtx->count++;
            return MUTEX_SUCCESS;
        }
    }
    else if (mtx->type == MUTEX_ERRORCHECK)
    {
        if (crt_atomic_load_long(&mtx->owner_tid) == tid)
            return MUTEX_EDEADLK;
    }

    __crt_lock_acquire(&mtx->lock);
    crt_atomic_store_long(&mtx->owner_tid, tid);
    mtx->count = 1;

    return MUTEX_SUCCESS;
}

int mutex_trylock(mutex_t *mtx)
{
    if (mtx == nullptr)
        return MUTEX_ERROR;

    long tid = mutex_current_tid();

    if (mtx->type == MUTEX_RECURSIVE)
    {
        if (crt_atomic_load_long(&mtx->owner_tid) == tid)
        {
            if (mtx->count == UINT32_MAX)
                return MUTEX_ERROR;

            mtx->count++;
            return MUTEX_SUCCESS;
        }
    }
    else if (mtx->type == MUTEX_ERRORCHECK)
    {
        if (crt_atomic_load_long(&mtx->owner_tid) == tid)
            return MUTEX_EDEADLK;
    }

    if (!__crt_lock_tryacquire(&mtx->lock))
        return MUTEX_BUSY;

    crt_atomic_store_long(&mtx->owner_tid, tid);
    mtx->count = 1;

    return MUTEX_SUCCESS;
}

int mutex_unlock(mutex_t *mtx)
{
    if (mtx == nullptr)
        return MUTEX_ERROR;

    long tid = mutex_current_tid();

    if (crt_atomic_load_long(&mtx->owner_tid) != tid)
        return MUTEX_EPERM;

    if (mtx->count > 1)
    {
        mtx->count--;
        return MUTEX_SUCCESS;
    }

    mtx->count = 0;
    crt_atomic_store_long(&mtx->owner_tid, 0);
    __crt_lock_release(&mtx->lock);

    return MUTEX_SUCCESS;
}

bool mutex_is_locked(const mutex_t *mtx)
{
    if (mtx == nullptr)
        return false;

    return crt_atomic_load_long(&mtx->owner_tid) != 0;
}

bool mutex_is_owned_by_current_thread(const mutex_t *mtx)
{
    if (mtx == nullptr)
        return false;

    return crt_atomic_load_long(&mtx->owner_tid) == mutex_current_tid();
}

long mutex_get_owner(const mutex_t *mtx)
{
    return mtx ? crt_atomic_load_long(&mtx->owner_tid) : 0;
}

u32 mutex_get_count(const mutex_t *mtx)
{
    return mtx ? mtx->count : 0;
}