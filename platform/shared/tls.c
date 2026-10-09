// ++C C Runtime Library (libminicrt) | Platform (Linux/FreeBSD)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_thread.h"
#include <mem.h>

#define AT_NULL   0
#define AT_PHDR   3
#define AT_PHENT  4
#define AT_PHNUM  5
#define AT_BASE   7
#define PT_PHDR   6
#define PT_TLS    7

#if defined(__x86_64__) || defined(__aarch64__)
typedef struct { u32 p_type, p_flags; u64 p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align; } xxc_phdr_t;
#else
typedef struct { u32 p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align; } xxc_phdr_t;
#endif

#define PT_DYNAMIC 2
#define DT_DEBUG   21

#if defined(__x86_64__) || defined(__aarch64__)
typedef struct { unsigned char e_ident[16]; u16 e_type, e_machine; u32 e_version; u64 e_entry, e_phoff, e_shoff; u32 e_flags; u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx; } xxc_ehdr_t;
typedef struct { i64 d_tag; u64 d_val; } xxc_dyn_t;
typedef u64 xxc_addr_t;
#else
typedef struct { unsigned char e_ident[16]; u16 e_type, e_machine; u32 e_version, e_entry, e_phoff, e_shoff, e_flags; u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx; } xxc_ehdr_t;
typedef struct { i32 d_tag; u32 d_val; } xxc_dyn_t;
typedef u32 xxc_addr_t;
#endif

typedef struct xxc_link_map {
    xxc_addr_t           l_addr;
    char                *l_name;
    xxc_dyn_t           *l_ld;
    struct xxc_link_map *l_next, *l_prev;
} xxc_link_map_t;

typedef struct { int r_version; xxc_link_map_t *r_map; } xxc_r_debug_t;

#define XXC_TCB_SIZE 16

__thread struct xxc_thread *__xxc_self;
struct xxc_thread           __xxc_main_thread;

static struct {
    const char *image;
    size_t      filesz;
    size_t      memsz;
    size_t      align;
    size_t      off;
} g_tls = { nullptr, 0, 0, 1, 0 };

#if defined(__i386__)
static u32 g_gs_entry;
#endif

static inline uintptr_t align_up_u(uintptr_t v, uintptr_t a)   { return (v + a - 1) & ~(a - 1); }
static inline uintptr_t align_down_u(uintptr_t v, uintptr_t a) { return v & ~(a - 1); }

static void tls_scan(int argc, char **argv)
{
    char **envp = argv + argc + 1;
    while (*envp) envp++;
    const unsigned long *av = (const unsigned long *)(envp + 1);

    const xxc_phdr_t *ph = nullptr;
    size_t phnum = 0;
    for (; av[0] != AT_NULL; av += 2)
    {
        if (av[0] == AT_PHDR)  ph = (const xxc_phdr_t *)av[1];
        if (av[0] == AT_PHNUM) phnum = (size_t)av[1];
    }
    if (ph == nullptr) return;

    uintptr_t bias = 0;
    for (size_t i = 0; i < phnum; i++)
        if (ph[i].p_type == PT_PHDR) { bias = (uintptr_t)ph - (uintptr_t)ph[i].p_vaddr; break; }

    for (size_t i = 0; i < phnum; i++)
    {
        if (ph[i].p_type != PT_TLS) continue;
        g_tls.image  = (const char *)(bias + (uintptr_t)ph[i].p_vaddr);
        g_tls.filesz = (size_t)ph[i].p_filesz;
        g_tls.memsz  = (size_t)ph[i].p_memsz;
        g_tls.align  = ph[i].p_align ? (size_t)ph[i].p_align : 1;
        break;
    }

#if defined(__aarch64__)
    g_tls.off = XXC_TCB_SIZE + ((-(uintptr_t)XXC_TCB_SIZE + (uintptr_t)g_tls.image) & (g_tls.align - 1));
#else
    g_tls.off = g_tls.memsz + ((-g_tls.memsz - (uintptr_t)g_tls.image) & (g_tls.align - 1));
#endif
}

size_t __xxc_tls_area_size(void)
{
    size_t a = g_tls.align < 16 ? 16 : g_tls.align;
#if defined(__aarch64__)
    return g_tls.off + g_tls.memsz + a + XXC_TCB_SIZE;
#else
    return g_tls.off + a + XXC_TCB_SIZE * 2;
#endif
}

char *__xxc_tls_setup(char *region_end, void **tp_out)
{
    uintptr_t a = g_tls.align < 16 ? 16 : g_tls.align;
    char *tp, *block, *lowest;

#if defined(__aarch64__)
    tp    = (char *)align_down_u((uintptr_t)region_end - (g_tls.off + g_tls.memsz), a);
    block = tp + g_tls.off;
    lowest = tp;
#else
    tp    = (char *)align_down_u((uintptr_t)region_end - XXC_TCB_SIZE, a);
    block = tp - g_tls.off;
    lowest = g_tls.memsz ? block : tp;
    *(void **)tp = tp;
#endif

    if (g_tls.memsz)
    {
        if (g_tls.filesz)
            memcpy(block, g_tls.image, g_tls.filesz);
        memset(block + g_tls.filesz, 0, g_tls.memsz - g_tls.filesz);
    }

    *tp_out = tp;
    return lowest;
}

static void set_thread_pointer(void *tp)
{
#if defined(__x86_64__) && defined(__FreeBSD__)
    unsigned long base = (unsigned long)tp;
    (void)syscall(XXC_SYS_sysarch, 129L /* AMD64_SET_FSBASE */, (long)&base);
#elif defined(__x86_64__)
    (void)syscall(__NR_arch_prctl, 0x1002L, (long)tp);
#elif defined(__i386__)
    xxc_user_desc_t d;
    d.entry_number = (u32)-1;
    d.base_addr    = (u32)(uintptr_t)tp;
    d.limit        = 0xfffff;
    d.flags        = 0x51;
    (void)syscall(__NR_set_thread_area, (long)&d);
    g_gs_entry = d.entry_number;
    unsigned sel = (d.entry_number << 3) | 3;
    __asm__ volatile("movw %w0, %%gs" :: "r"(sel) : "memory");
#elif defined(__aarch64__)
    __asm__ volatile("msr tpidr_el0, %0" :: "r"(tp) : "memory");
#endif
}

#if defined(__i386__)
void __xxc_fill_user_desc(struct xxc_thread *t, void *tp)
{
    t->user_desc.entry_number = g_gs_entry;
    t->user_desc.base_addr    = (u32)(uintptr_t)tp;
    t->user_desc.limit        = 0xfffff;
    t->user_desc.flags        = 0x51;
}
#endif

static int has_interp(int argc, char **argv)
{
    char **envp = argv + argc + 1;
    while (*envp) envp++;
    for (const unsigned long *av = (const unsigned long *)(envp + 1); av[0] != AT_NULL; av += 2)
        if (av[0] == AT_BASE) return av[1] != 0;
    return 0;
}

static void *get_thread_pointer(void)
{
    void *tp;
#if defined(__x86_64__)
    __asm__ volatile("movq %%fs:0, %0" : "=r"(tp));
#elif defined(__i386__)
    __asm__ volatile("movl %%gs:0, %0" : "=r"(tp));
#elif defined(__aarch64__)
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp));
#endif
    return tp;
}

static void tls_accum(const xxc_phdr_t *ph, size_t phnum, size_t *total, size_t *maxal)
{
    for (size_t i = 0; i < phnum; i++)
    {
        if (ph[i].p_type != PT_TLS) continue;
        size_t al = ph[i].p_align ? (size_t)ph[i].p_align : 1;
        *total += (size_t)align_up_u((uintptr_t)ph[i].p_memsz, al) + al;
        if (al > *maxal) *maxal = al;
    }
}

static int tls_snapshot_dynamic(int argc, char **argv)
{
    char **envp = argv + argc + 1;
    while (*envp) envp++;
    const unsigned long *av = (const unsigned long *)(envp + 1);

    const xxc_phdr_t *ph = nullptr;
    size_t phnum = 0;
    for (; av[0] != AT_NULL; av += 2)
    {
        if (av[0] == AT_PHDR)  ph = (const xxc_phdr_t *)av[1];
        if (av[0] == AT_PHNUM) phnum = (size_t)av[1];
    }
    if (ph == nullptr) return 0;

    uintptr_t bias = 0;
    for (size_t i = 0; i < phnum; i++)
        if (ph[i].p_type == PT_PHDR) { bias = (uintptr_t)ph - (uintptr_t)ph[i].p_vaddr; break; }

    xxc_r_debug_t *rd = nullptr;
    for (size_t i = 0; i < phnum && !rd; i++)
    {
        if (ph[i].p_type != PT_DYNAMIC) continue;
        for (const xxc_dyn_t *d = (const xxc_dyn_t *)(bias + (uintptr_t)ph[i].p_vaddr); d->d_tag != 0; d++)
            if (d->d_tag == DT_DEBUG) { rd = (xxc_r_debug_t *)(uintptr_t)d->d_val; break; }
    }
    if (rd == nullptr || !rd->r_map) return 0;

    size_t total = 0, maxal = 1;

    tls_accum(ph, phnum, &total, &maxal);

    for (const xxc_link_map_t *m = rd->r_map->l_next; m; m = m->l_next)
    {
        const xxc_ehdr_t *eh = (const xxc_ehdr_t *)(uintptr_t)m->l_addr;
        if (eh == nullptr || eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E' ||
            eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F')
            continue;
        tls_accum((const xxc_phdr_t *)((const char *)eh + eh->e_phoff), eh->e_phnum, &total, &maxal);
    }

    if (!total) return 0;

    size_t a = maxal < 16 ? 16 : maxal;
    size_t n = (size_t)align_up_u(total, a);

    char *snap = (char *)sys_mmap(nullptr, align_up_u(n, 4096), PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANON, -1, 0);
    if (snap == nullptr) return 0;

    const char *tp = (const char *)get_thread_pointer();
#if defined(__aarch64__)
    memcpy(snap, tp + XXC_TCB_SIZE, n);
    g_tls.off = XXC_TCB_SIZE;
#else
    memcpy(snap, tp - n, n);
    g_tls.off = n;
#endif
    g_tls.image  = snap;
    g_tls.filesz = n;
    g_tls.memsz  = n;
    g_tls.align  = a;
    return 1;
}

void __xxc_platform_init(int argc, char **argv)
{
    if (has_interp(argc, argv))
    {
        (void)tls_snapshot_dynamic(argc, argv);
        struct xxc_thread *t = &__xxc_main_thread;
        t->self = t;
        atomic_i32_init(&t->tid, (i32)sys_gettid());
        atomic_i32_init(&t->state, XXC_THREAD_DETACHED);
        __xxc_self = t;
        return;
    }
    tls_scan(argc, argv);

    size_t len = align_up_u(__xxc_tls_area_size() + 64, 4096);
    char *area = (char *)sys_mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (area == nullptr)
    {
        static const char msg[] = "xxc: cannot allocate TLS block\n";
        sys_write(2, msg, sizeof(msg) - 1);
        sys_exit(127);
    }

    void *tp;
    (void)__xxc_tls_setup(area + len, &tp);
    set_thread_pointer(tp);

    struct xxc_thread *t = &__xxc_main_thread;
    t->self  = t;
    atomic_i32_init(&t->tid, (i32)sys_gettid());
    atomic_i32_init(&t->state, XXC_THREAD_DETACHED);
    __xxc_self = t;
}