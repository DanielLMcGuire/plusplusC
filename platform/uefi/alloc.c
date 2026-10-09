// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"
#include <mem.h>

typedef struct { void *raw; size_t size; } hdr_t;

void *__uefi_malloc(size_t size)
{
    if (size == 0) size = 1;
    if (size > ((size_t)-1) / 2) return nullptr;

    void *raw = nullptr;
    if (__uefi_st->BootServices->AllocatePool(EfiLoaderData, size + sizeof(hdr_t) + 16, &raw) != EFI_SUCCESS)
        return nullptr;

    uintblock_t user = ((uintblock_t)raw + sizeof(hdr_t) + 15) & ~(uintblock_t)15;
    hdr_t *h = (hdr_t *)user - 1;
    h->raw  = raw;
    h->size = size;
    return (void *)user;
}

void *__uefi_calloc(size_t count, size_t size)
{
    size_t total = count * size;
    if (count != 0 && total / count != size) return nullptr;
    void *p = __uefi_malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void __uefi_free(void *block)
{
    if (block == nullptr) return;
    __uefi_st->BootServices->FreePool(((hdr_t *)block - 1)->raw);
}

size_t __uefi_usable_size(void *block)
{
    return block ? ((hdr_t *)block - 1)->size : 0;
}

void *__uefi_realloc(void *block, size_t size)
{
    if (block == nullptr) return __uefi_malloc(size);
    if (size == 0) { __uefi_free(block); return nullptr; }

    size_t old = __uefi_usable_size(block);
    if (size <= old) return block;

    void *n = __uefi_malloc(size);
    if (n == nullptr) return nullptr;
    memcpy(n, block, old);
    __uefi_free(block);
    return n;
}
