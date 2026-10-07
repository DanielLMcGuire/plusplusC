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
    if (size == 0) size = 1;
    return HeapAlloc(heap, 0, (SIZE_T)size);
}
#elif defined(XXC_RAWSYS)
void* malloc(size_t size)
{
    return __linux_malloc(size);
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
    return __linux_calloc(count, size);
#endif
}

#ifdef _WIN32
void* realloc(void *ptr, size_t size)
{
    if (!ptr)
    {
        if (size == 0) size = 1;
        return HeapAlloc(heap, 0, (SIZE_T)size);
    }

    if (size == 0)
    {
        HeapFree(heap, 0, ptr);
        return NULL;
    }

    return HeapReAlloc(heap, 0, ptr, (SIZE_T)size);
}
#elif defined(XXC_RAWSYS)
void* realloc(void *ptr, size_t size)
{
    return __linux_realloc(ptr, size);
}
#endif

#ifdef _WIN32
void free(void *ptr) 
{
    if (!ptr) 
        return;

    HeapFree(heap, 0, ptr);
}
#elif defined(XXC_RAWSYS)
void free(void *ptr)
{
    __linux_free(ptr);
}
#endif

#ifdef _WIN32
size_t malloc_usable_size(void *ptr)
{
    return ptr ? (size_t)HeapSize(heap, 0, ptr) : 0;
}
#elif defined(XXC_RAWSYS)
size_t malloc_usable_size(void *ptr)
{
    return __linux_usable_size(ptr);
}
#endif

#endif /* __UEFI__ */
