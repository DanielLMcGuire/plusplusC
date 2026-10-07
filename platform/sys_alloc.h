// ++C C Runtime Library (libminicrt) | Platform (Linux/FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SYS_ALLOC_H
#define SYS_ALLOC_H

#include <def.h>
#include "xxc_sys.h"

void  *__xxc_malloc(size_t size);
void  *__xxc_calloc(size_t count, size_t size);
void  *__xxc_realloc(void *ptr, size_t size);
void   __xxc_free(void *ptr);
size_t __xxc_usable_size(void *ptr);

void   __xxc_heap_thread_exit(void);

#endif /* SYS_ALLOC_H */
