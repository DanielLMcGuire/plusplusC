// ++C C Runtime Library (libminicrt) | Platform (win32)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <def.h>
#include <alloc.h>
#include <mem.h>
#include <crt_lock.h>
#include <windows.h>

struct __emutls_object {
    uintptr_t size;
    uintptr_t align;
    union { uintptr_t index; void *address; } loc;
    void *templ;
};

static crt_lock_t   g_index_lock = CRT_LOCK_INIT;
static uintptr_t    g_next_index = 0;

typedef struct { void **slots; size_t cap; } emutls_array_t;

static DWORD get_tls_slot(void)
{
    static volatile LONG slot = -1;
    LONG v = slot;
    if (v == -1)
    {
        DWORD n = TlsAlloc();
        LONG prev = InterlockedCompareExchange(&slot, (LONG)n, -1);
        if (prev != -1)
        { 
            TlsFree(n);
            n = (DWORD)prev; 
        }
        v = (LONG)n;
    }
    return (DWORD)v;
}

static XXC_NORETURN void emutls_oom(void)
{
    static const char msg[] = "xxc: out of memory (thread-local storage)\n";
    DWORD written;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg) - 1, &written, NULL);
    ExitProcess(127);
}

static void *emutls_aligned_alloc(size_t size, size_t align)
{
    if (align < sizeof(void *)) align = sizeof(void *);
    void *raw = malloc(size + align - 1 + sizeof(void *));
    if (!raw) return NULL;
    uintptr_t p = ((uintptr_t)raw + sizeof(void *) + align - 1) & ~(uintptr_t)(align - 1);
    ((void **)p)[-1] = raw;
    return (void *)p;
}

static void emutls_aligned_free(void *p)
{
    if (p) free(((void **)p)[-1]);
}

void *__emutls_get_address(void *object)
{
    struct __emutls_object *obj = (struct __emutls_object *)object;

    uintptr_t index = obj->loc.index;
    if (XXC_UNLIKELY(index == 0))
    {
        __crt_lock_acquire(&g_index_lock);
        index = obj->loc.index;
        if (index == 0)
        {
            index = ++g_next_index;
            obj->loc.index = index;
        }
        __crt_lock_release(&g_index_lock);
    }

    DWORD slot = get_tls_slot();
    emutls_array_t *arr = (emutls_array_t *)TlsGetValue(slot);
    if (!arr)
    {
        arr = (emutls_array_t *)calloc(1, sizeof(*arr));
        if (!arr) emutls_oom();
        TlsSetValue(slot, arr);
    }

    if (index > arr->cap)
    {
        size_t newcap = index + 16;
        void **p = (void **)realloc(arr->slots, newcap * sizeof(void *));
        if (!p) emutls_oom();
        memset(p + arr->cap, 0, (newcap - arr->cap) * sizeof(void *));
        arr->slots = p;
        arr->cap = newcap;
    }

    void *val = arr->slots[index - 1];
    if (!val)
    {
        val = emutls_aligned_alloc((size_t)obj->size, (size_t)obj->align);
        if (!val) emutls_oom();
        if (obj->templ)
            memcpy(val, obj->templ, (size_t)obj->size);
        else
            memset(val, 0, (size_t)obj->size);
        arr->slots[index - 1] = val;
    }
    return val;
}

void __xxc_emutls_thread_exit(void)
{
    DWORD slot = get_tls_slot();
    emutls_array_t *arr = (emutls_array_t *)TlsGetValue(slot);
    if (!arr) return;
    for (size_t i = 0; i < arr->cap; i++)
        emutls_aligned_free(arr->slots[i]);
    free(arr->slots);
    free(arr);
    TlsSetValue(slot, NULL);
}
