// ++C C Runtime Library (libminicrt) | Platform (Linux/FreeBSD) - allocator interface
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef SYS_ALLOC_H
#define SYS_ALLOC_H

#include <def.h>
#include "xxc_sys.h"

void  *__linux_malloc(size_t size);
void  *__linux_calloc(size_t count, size_t size);
void  *__linux_realloc(void *ptr, size_t size);
void   __linux_free(void *ptr);
size_t __linux_usable_size(void *ptr);

void   __linux_heap_thread_exit(void);

#endif /* SYS_ALLOC_H */
