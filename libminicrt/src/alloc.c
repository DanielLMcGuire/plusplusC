// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <alloc.h>
#include <def.h>
#include <mem.h>
#ifdef __UEFI__
#include <sys_uefi.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(XXC_RAWSYS)
#include <xxc_sys.h>
#include <sys_alloc.h>
#endif

#ifdef __UEFI__

void* malloc(size_t size) 
{
    return __uefi_malloc(size);    
}

void* calloc(size_t n, size_t size) 
{ 
    return __uefi_calloc(n, size); 
}

void* realloc(void *p, size_t size) 
{ 
    return __uefi_realloc(p, size); 
}

void free(void *p) 
{ 
    __uefi_free(p); 
}

size_t malloc_usable_size(void *p) 
{ 
    return __uefi_usable_size(p); 
}

#else

#ifdef _WIN32

XXC_DYN_EXPORT HANDLE heap = INVALID_HANDLE_VALUE;

void* malloc(size_t size)
{
    return HeapAlloc(heap, 0, (SIZE_T)size);
}
#elif defined(XXC_RAWSYS)
void* malloc(size_t size)
{
    return __xxc_malloc(size);
}
#endif

void* calloc(size_t count, size_t size)
{
#ifdef _WIN32
    size_t total = count * size;

    if (count != 0 && total / count != size)
        return NULL;
    return HeapAlloc(heap, HEAP_ZERO_MEMORY, (SIZE_T)(total ? total : 1));
#elif defined(XXC_RAWSYS)
    return __xxc_calloc(count, size);
#endif
}

#ifdef _WIN32
void* realloc(void *block, size_t size)
{
    if (!block)
    {
        if (size == 0) size = 1;
        return HeapAlloc(heap, 0, (SIZE_T)size);
    }

    if (size == 0)
    {
        HeapFree(heap, 0, block);
        return NULL;
    }

    return HeapReAlloc(heap, 0, block, (SIZE_T)size);
}
#elif defined(XXC_RAWSYS)
void* realloc(void *block, size_t size)
{
    return __xxc_realloc(block, size);
}
#endif

#ifdef _WIN32
void free(void *block) 
{
    if (!block) 
        return;

    HeapFree(heap, 0, block);
}
#elif defined(XXC_RAWSYS)
void free(void *block)
{
    __xxc_free(block);
}
#endif

#ifdef _WIN32
size_t malloc_usable_size(void *block)
{
    return block ? (size_t)HeapSize(heap, 0, block) : 0;
}
#elif defined(XXC_RAWSYS)
size_t malloc_usable_size(void *block)
{
    return __xxc_usable_size(block);
}
#endif

#endif /* __UEFI__ */
