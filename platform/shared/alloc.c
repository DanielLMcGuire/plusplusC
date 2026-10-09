// ++C C Runtime Library (libminicrt) | Platform (Linux/FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_alloc.h"
#include <atomic.h>
#include <crt_lock.h>
#include <mem.h>

#define XXC_PAGE_SHIFT     16
#define XXC_PAGE_SIZE      ((size_t)1 << XXC_PAGE_SHIFT)
#define XXC_PAGE_MASK      (~(uintptr_t)(XXC_PAGE_SIZE - 1))
#define XXC_PAGE_HDR       128
#define XXC_CHUNK_PAGES    16
#define XXC_SMALL_MAX      8192
#define XXC_NUM_CLASSES    32
#define XXC_SPARE_MAX      2
#define XXC_POOL_KEEP      64
#define XXC_LARGE_HDR      64
#define XXC_LARGE_SLOTS    16
#define XXC_LARGE_CACHE_MAX_ITEM ((size_t)2 << 20)
#define XXC_MAX_REQUEST    (((size_t)-1) >> 2)

#define XXC_MAGIC_SMALL    0x58414C53u
#define XXC_MAGIC_LARGE    0x58414C4Cu

typedef struct xxc_block { struct xxc_block *next; } xxc_block;

typedef struct xxc_page {
    u32                 magic;
    u8                  cls;
    u8                  full;
    u16                 pad;
    struct xxc_heap    *heap;
    xxc_block          *free;
    struct xxc_page    *next, *prev;
    u32                 block_size;
    u32                 capacity;
    u32                 fresh;
    u32                 used;
} xxc_page;

typedef struct xxc_large {
    u32                 magic;
    u32                 pad;
    size_t              map_size;
} xxc_large;

typedef struct xxc_heap {
    xxc_page           *pages[XXC_NUM_CLASSES];
    xxc_page           *spare;
    u32                 nspare;
    struct xxc_heap    *next_idle;
    _Alignas(XXC_CACHELINE) atomic_ptr_t delayed;
    atomic_u32_t        idle;
} xxc_heap;

_Static_assert(sizeof(xxc_page) <= XXC_PAGE_HDR, "page header too large");
_Static_assert(sizeof(xxc_large) <= XXC_LARGE_HDR, "large header too large");

static crt_lock_t   g_lock = CRT_LOCK_INIT;
static xxc_heap    *g_idle_heaps;
static xxc_page    *g_page_pool;
static size_t       g_pool_count;
static char        *g_chunk_cur, *g_chunk_end;
static char        *g_heap_cur,  *g_heap_end;
static struct { char *base; size_t size; } g_large_cache[XXC_LARGE_SLOTS];

static __thread xxc_heap *t_heap;

static XXC_NORETURN void fatal(const char *msg)
{
    size_t n = 0;
    while (msg[n]) n++;
    sys_write(2, msg, n);
    __builtin_trap();
}

static inline size_t align_up_sz(size_t v, size_t a) { return (v + a - 1) & ~(a - 1); }
static inline xxc_page *page_of(const void *p) { return (xxc_page *)((uintptr_t)p & XXC_PAGE_MASK); }

static inline unsigned class_of(size_t size)
{
    if (size <= 128)
        return (unsigned)((size + 15) >> 4) - 1;
    unsigned s = (unsigned)size - 1;
    unsigned m = 31u - (unsigned)__builtin_clz(s);
    return 8 + (m - 7) * 4 + ((s >> (m - 2)) & 3);
}

static inline u32 class_size(unsigned cls)
{
    if (cls < 8)
        return (cls + 1) * 16;
    unsigned k = cls - 8, m = 7 + k / 4, j = k % 4;
    return (1u << m) + ((1u << m) / 4) * (j + 1);
}

static char *os_map_aligned(size_t size)
{
    size_t total = size + XXC_PAGE_SIZE;
    if (total < size) return nullptr;
    char *raw = (char *)sys_mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (raw == nullptr) return nullptr;

    uintptr_t a = ((uintptr_t)raw + XXC_PAGE_SIZE - 1) & XXC_PAGE_MASK;
    size_t head = a - (uintptr_t)raw;
    size_t tail = total - head - size;
    if (head) sys_munmap(raw, head);
    if (tail) sys_munmap((char *)a + size, tail);
    return (char *)a;
}

static xxc_page *pool_get_locked(void)
{
    xxc_page *p = g_page_pool;
    if (p != nullptr)
    {
        g_page_pool = p->next;
        g_pool_count--;
        return p;
    }
    if (g_chunk_cur == g_chunk_end)
    {
        char *c = os_map_aligned(XXC_CHUNK_PAGES * XXC_PAGE_SIZE);
        if (c == nullptr) return nullptr;
        g_chunk_cur = c;
        g_chunk_end = c + XXC_CHUNK_PAGES * XXC_PAGE_SIZE;
    }
    p = (xxc_page *)g_chunk_cur;
    g_chunk_cur += XXC_PAGE_SIZE;
    return p;
}

static void pool_put_locked(xxc_page *p)
{
    if (g_pool_count >= XXC_POOL_KEEP)
        sys_madvise(p, XXC_PAGE_SIZE, MADV_DONTNEED);
    p->next = g_page_pool;
    g_page_pool = p;
    g_pool_count++;
}

static xxc_heap *heap_struct_new_locked(void)
{
    size_t sz = align_up_sz(sizeof(xxc_heap), XXC_CACHELINE);
    if ((size_t)(g_heap_end - g_heap_cur) < sz)
    {
        char *m = (char *)sys_mmap(nullptr, XXC_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (m == nullptr) return nullptr;
        g_heap_cur = m;
        g_heap_end = m + XXC_PAGE_SIZE;
    }
    xxc_heap *h = (xxc_heap *)g_heap_cur;
    g_heap_cur += sz;
    return h;
}

static inline void list_unlink(xxc_heap *h, xxc_page *pg)
{
    if (pg->prev != nullptr) pg->prev->next = pg->next; else h->pages[pg->cls] = pg->next;
    if (pg->next != nullptr) pg->next->prev = pg->prev;
    pg->next = pg->prev = nullptr;
}

static inline void list_push(xxc_heap *h, xxc_page *pg)
{
    pg->prev = nullptr;
    pg->next = h->pages[pg->cls];
    if (pg->next != nullptr) pg->next->prev = pg;
    h->pages[pg->cls] = pg;
}

static void page_maybe_retire(xxc_heap *h, xxc_page *pg, bool locked)
{
    if (h->pages[pg->cls] == pg && pg->next == nullptr)
        return;
    list_unlink(h, pg);

    if (locked) { pool_put_locked(pg); return; }
    if (h->nspare < XXC_SPARE_MAX)
    {
        pg->next = h->spare;
        h->spare = pg;
        h->nspare++;
        return;
    }
    __crt_lock_acquire(&g_lock);
    pool_put_locked(pg);
    __crt_lock_release(&g_lock);
}

static inline void page_free_local(xxc_heap *h, xxc_page *pg, xxc_block *b, bool locked)
{
    b->next = pg->free;
    pg->free = b;
    pg->used--;
    if (XXC_UNLIKELY(pg->full))
    {
        pg->full = 0;
        list_push(h, pg);
    }
    if (XXC_UNLIKELY(pg->used == 0))
        page_maybe_retire(h, pg, locked);
}

static void heap_drain_delayed(xxc_heap *h, bool locked)
{
    if (!atomic_ptr_load_explicit(&h->delayed, ATOMIC_RELAXED))
        return;
    xxc_block *b = (xxc_block *)atomic_ptr_exchange_explicit(&h->delayed, nullptr, ATOMIC_ACQUIRE);
    while (b)
    {
        xxc_block *next = b->next;
        page_free_local(h, page_of(b), b, locked);
        b = next;
    }
}

static xxc_page *page_new(xxc_heap *h, unsigned cls)
{
    xxc_page *pg = h->spare;
    if (pg != nullptr)
    {
        h->spare = pg->next;
        h->nspare--;
    }
    else
    {
        __crt_lock_acquire(&g_lock);
        pg = pool_get_locked();
        __crt_lock_release(&g_lock);
        if (pg == nullptr) return nullptr;
    }
    pg->magic      = XXC_MAGIC_SMALL;
    pg->heap       = h;
    pg->cls        = (u8)cls;
    pg->full       = 0;
    pg->block_size = class_size(cls);
    pg->capacity   = (u32)((XXC_PAGE_SIZE - XXC_PAGE_HDR) / pg->block_size);
    pg->fresh      = 0;
    pg->used       = 0;
    pg->free       = nullptr;
    list_push(h, pg);
    return pg;
}

static XXC_NOINLINE xxc_heap *heap_acquire(void)
{
    __crt_lock_acquire(&g_lock);
    xxc_heap *h = g_idle_heaps;
    if (h != nullptr)
        g_idle_heaps = h->next_idle;
    else
        h = heap_struct_new_locked();
    if (h != nullptr)
        atomic_u32_store(&h->idle, 0);
    __crt_lock_release(&g_lock);

    t_heap = h;
    return h;
}

void __xxc_heap_thread_exit(void)
{
    xxc_heap *h = t_heap;
    if (h == nullptr) return;

    heap_drain_delayed(h, false);

    for (unsigned c = 0; c < XXC_NUM_CLASSES; c++)
    {
        xxc_page *pg = h->pages[c];
        while (pg != nullptr)
        {
            xxc_page *next = pg->next;
            if (pg->used == 0)
            {
                list_unlink(h, pg);
                __crt_lock_acquire(&g_lock);
                pool_put_locked(pg);
                __crt_lock_release(&g_lock);
            }
            pg = next;
        }
    }

    __crt_lock_acquire(&g_lock);
    while (h->spare)
    {
        xxc_page *pg = h->spare;
        h->spare = pg->next;
        pool_put_locked(pg);
    }
    h->nspare = 0;
    h->next_idle = g_idle_heaps;
    g_idle_heaps = h;
    atomic_u32_store(&h->idle, 1);
    __crt_lock_release(&g_lock);

    t_heap = nullptr;
}

static XXC_NOINLINE void *small_alloc_slow(xxc_heap *h, unsigned cls)
{
    heap_drain_delayed(h, false);

    for (;;)
    {
        xxc_page *pg = h->pages[cls];
        if (pg == nullptr)
        {
            pg = page_new(h, cls);
            if (pg == nullptr) return nullptr;
        }
        xxc_block *b = pg->free;
        if (b)
        {
            pg->free = b->next;
            pg->used++;
            return b;
        }
        if (pg->fresh < pg->capacity)
        {
            b = (xxc_block *)((char *)pg + XXC_PAGE_HDR + (size_t)pg->fresh * pg->block_size);
            pg->fresh++;
            pg->used++;
            return b;
        }
        list_unlink(h, pg);
        pg->full = 1;
    }
}

static inline void *small_alloc(size_t size)
{
    xxc_heap *h = t_heap;
    if (XXC_UNLIKELY(h == nullptr))
    {
        h = heap_acquire();
        if (h == nullptr) return nullptr;
    }
    unsigned cls = class_of(size);
    xxc_page *pg = h->pages[cls];
    if (XXC_LIKELY(pg != nullptr))
    {
        xxc_block *b = pg->free;
        if (XXC_LIKELY(b != nullptr))
        {
            pg->free = b->next;
            pg->used++;
            return b;
        }
    }
    return small_alloc_slow(h, cls);
}

static void free_remote(xxc_heap *h, xxc_block *b)
{
    if (XXC_UNLIKELY(atomic_u32_load_explicit(&h->idle, ATOMIC_ACQUIRE)))
    {
        __crt_lock_acquire(&g_lock);
        if (atomic_u32_load(&h->idle))
        {
            heap_drain_delayed(h, true);
            page_free_local(h, page_of(b), b, true);
            __crt_lock_release(&g_lock);
            return;
        }
        __crt_lock_release(&g_lock);
    }

    void *old = atomic_ptr_load_explicit(&h->delayed, ATOMIC_RELAXED);
    do {
        b->next = (xxc_block *)old;
    } while (!atomic_ptr_cas_weak_explicit(&h->delayed, &old, b, ATOMIC_RELEASE, ATOMIC_RELAXED));
}

static void *large_alloc(size_t size, bool *zeroed)
{
    if (size > XXC_MAX_REQUEST) return nullptr;
    size_t need = align_up_sz(size + XXC_LARGE_HDR, XXC_PAGE_SIZE);

    char *base = nullptr;
    size_t msize = 0;

    __crt_lock_acquire(&g_lock);
    int best = -1;
    for (int i = 0; i < XXC_LARGE_SLOTS; i++)
    {
        size_t s = g_large_cache[i].size;
        if (g_large_cache[i].base && s >= need && s <= need * 2 && (best < 0 || s < g_large_cache[best].size))
            best = i;
    }
    if (best >= 0)
    {
        base = g_large_cache[best].base;
        msize = g_large_cache[best].size;
        g_large_cache[best].base = nullptr;
    }
    __crt_lock_release(&g_lock);

    if (base != nullptr)
    {
        if (zeroed) *zeroed = false;
    }
    else
    {
        base = os_map_aligned(need);
        if (base == nullptr) return nullptr;
        msize = need;
        if (zeroed) *zeroed = true;
    }

    xxc_large *L = (xxc_large *)base;
    L->magic = XXC_MAGIC_LARGE;
    L->map_size = msize;
    return base + XXC_LARGE_HDR;
}

static void large_free(xxc_large *L)
{
    size_t msize = L->map_size;
    L->magic = 0;

    if (msize <= XXC_LARGE_CACHE_MAX_ITEM)
    {
        __crt_lock_acquire(&g_lock);
        for (int i = 0; i < XXC_LARGE_SLOTS; i++)
        {
            if (g_large_cache[i].base == nullptr)
            {
                g_large_cache[i].base = (char *)L;
                g_large_cache[i].size = msize;
                __crt_lock_release(&g_lock);
                return;
            }
        }
        __crt_lock_release(&g_lock);
    }
    sys_munmap(L, msize);
}

static inline void *alloc_impl(size_t size, bool *zeroed)
{
    if (zeroed) *zeroed = false;
    if (XXC_UNLIKELY(size == 0)) size = 1;
    if (XXC_LIKELY(size <= XXC_SMALL_MAX))
        return small_alloc(size);
    return large_alloc(size, zeroed);
}

void *__xxc_malloc(size_t size)
{
    return alloc_impl(size, nullptr);
}

void *__xxc_calloc(size_t count, size_t size)
{
    size_t total = count * size;
    if (count != 0 && total / count != size)
        return nullptr;
    bool zeroed;
    void *p = alloc_impl(total, &zeroed);
    if (p && !zeroed)
        memset(p, 0, total);
    return p;
}

void __xxc_free(void *ptr)
{
    if (ptr == nullptr) return;
    xxc_page *pg = page_of(ptr);
    if (XXC_LIKELY(pg->magic == XXC_MAGIC_SMALL))
    {
        xxc_heap *h = pg->heap;
        if (XXC_LIKELY(h == t_heap))
            page_free_local(h, pg, (xxc_block *)ptr, false);
        else
            free_remote(h, (xxc_block *)ptr);
        return;
    }
    if (pg->magic == XXC_MAGIC_LARGE)
    {
        large_free((xxc_large *)pg);
        return;
    }
    fatal("free(): invalid pointer\n");
}

size_t __xxc_usable_size(void *ptr)
{
    if (ptr == nullptr) return 0;
    xxc_page *pg = page_of(ptr);
    if (pg->magic == XXC_MAGIC_SMALL) return pg->block_size;
    if (pg->magic == XXC_MAGIC_LARGE) return ((xxc_large *)pg)->map_size - XXC_LARGE_HDR;
    fatal("malloc_usable_size(): invalid pointer\n");
}

void *__xxc_realloc(void *ptr, size_t size)
{
    if (ptr == nullptr) return __xxc_malloc(size);
    if (size == 0)
    {
        __xxc_free(ptr);
        return nullptr;
    }

    size_t usable = __xxc_usable_size(ptr);
    if (size <= usable && size >= usable / 2)
        return ptr;

    void *n = __xxc_malloc(size);
    if (n == nullptr) return nullptr;
    memcpy(n, ptr, usable < size ? usable : size);
    __xxc_free(ptr);
    return n;
}
