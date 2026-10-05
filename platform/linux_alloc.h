// ++C C Runtime Library (libminicrt) | Platform (Linux) - allocator interface
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef LINUX_ALLOC_H
#define LINUX_ALLOC_H

#include <def.h>
#include "sys_linux.h"

void  *__linux_malloc(size_t size);
void  *__linux_calloc(size_t count, size_t size);
void  *__linux_realloc(void *ptr, size_t size);
void   __linux_free(void *ptr);
size_t __linux_usable_size(void *ptr);

void   __linux_heap_thread_exit(void);

#endif /* LINUX_ALLOC_H */
