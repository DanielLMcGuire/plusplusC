// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef ALLOC_FUNCS_H
#define ALLOC_FUNCS_H

#include <def.h>

size_t malloc_usable_size(void *ptr);

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void *ptr, size_t size);

void free(void *ptr);

#if defined(__GNUC__) || defined(__clang__) 
#define alloca(size) __builtin_alloca(size);
#elif _MSC_VER
#define alloca _alloca
#endif

#endif