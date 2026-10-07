// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SEM_H
#define SEM_H

#include <def.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SEM_SUCCESS  =  0,
    SEM_ERROR    = -1,
    SEM_BUSY     = -2,
    SEM_TIMEDOUT = -3
};

#if defined(_WIN32)
    #include <windows.h>
    typedef struct { HANDLE handle; } sem_t;
#elif defined(XXC_RAWSYS)
    #include <atomic.h>
    typedef struct { atomic_i32_t count; } sem_t;
#endif

int sem_init(sem_t *sem, u32 initial);
int sem_destroy(sem_t *sem);

int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait_ms(sem_t *sem, u32 ms);

int sem_post(sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* SEM_H */
