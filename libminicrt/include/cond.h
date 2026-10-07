// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef COND_H
#define COND_H

#include <def.h>
#include <mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    COND_SUCCESS  =  0,
    COND_ERROR    = -1,
    COND_TIMEDOUT = -2
};

#if defined(_WIN32)
    #include <windows.h>
    typedef struct { CONDITION_VARIABLE cv; } cond_t;
    #define COND_INIT { CONDITION_VARIABLE_INIT }
#elif defined(XXC_RAWSYS)
    #include <atomic.h>
    typedef struct { atomic_u32_t seq; } cond_t;
    #define COND_INIT { ATOMIC_INIT(0) }
#endif

int cond_init(cond_t *cv);
int cond_destroy(cond_t *cv);

int cond_wait(cond_t *cv, mutex_t *mtx);
int cond_timedwait_ms(cond_t *cv, mutex_t *mtx, u32 ms);

void cond_signal(cond_t *cv);
void cond_broadcast(cond_t *cv);

#ifdef __cplusplus
}
#endif

#endif /* COND_H */
