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

XXC_DYN_EXPORT HANDLE _xxc_w32_heap = INVALID_HANDLE_VALUE;

void* malloc(size_t size)
{
    return HeapAlloc(_xxc_w32_heap, 0, (SIZE_T)size);
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
        return nullptr;
    return HeapAlloc(_xxc_w32_heap, HEAP_ZERO_MEMORY, (SIZE_T)(total ? total : 1));
#elif defined(XXC_RAWSYS)
    return __xxc_calloc(count, size);
#endif
}

#ifdef _WIN32
void* realloc(void *block, size_t size)
{
    if (block == nullptr)
        return malloc(size);    

    if (size == 0)
    {
        free(block);
        return nullptr;
    }

    SIZE_T have = HeapSize(_xxc_w32_heap, 0, block);
    if (have != (SIZE_T)-1 && (SIZE_T)size <= have)
        return block;

    return HeapReAlloc(_xxc_w32_heap, 0, block, (SIZE_T)size);
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
    if (block == nullptr) 
        return;

    HeapFree(_xxc_w32_heap, 0, block);
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
    return block ? (size_t)HeapSize(_xxc_w32_heap, 0, block) : 0;
}
#elif defined(XXC_RAWSYS)
size_t malloc_usable_size(void *block)
{
    return __xxc_usable_size(block);
}
#endif

#endif /* __UEFI__ */
