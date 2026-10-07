// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"
#include <mem.h>

typedef struct { void *raw; size_t size; } hdr_t;

void *__uefi_malloc(size_t size)
{
    if (size == 0) size = 1;
    if (size > ((size_t)-1) / 2) return NULL;

    void *raw = NULL;
    if (__uefi_st->BootServices->AllocatePool(EfiLoaderData, size + sizeof(hdr_t) + 16, &raw) != EFI_SUCCESS)
        return NULL;

    uintptr_t user = ((uintptr_t)raw + sizeof(hdr_t) + 15) & ~(uintptr_t)15;
    hdr_t *h = (hdr_t *)user - 1;
    h->raw  = raw;
    h->size = size;
    return (void *)user;
}

void *__uefi_calloc(size_t count, size_t size)
{
    size_t total = count * size;
    if (count != 0 && total / count != size) return NULL;
    void *p = __uefi_malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void __uefi_free(void *ptr)
{
    if (!ptr) return;
    __uefi_st->BootServices->FreePool(((hdr_t *)ptr - 1)->raw);
}

size_t __uefi_usable_size(void *ptr)
{
    return ptr ? ((hdr_t *)ptr - 1)->size : 0;
}

void *__uefi_realloc(void *ptr, size_t size)
{
    if (!ptr) return __uefi_malloc(size);
    if (size == 0) { __uefi_free(ptr); return NULL; }

    size_t old = __uefi_usable_size(ptr);
    if (size <= old) return ptr;

    void *n = __uefi_malloc(size);
    if (!n) return NULL;
    memcpy(n, ptr, old);
    __uefi_free(ptr);
    return n;
}
