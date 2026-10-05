// ++C C Runtime Library (libminicrt) | Platform (Linux)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../linux_thread.h"
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

#define XXC_TCB_SIZE 16

__thread struct xxc_thread *__xxc_self;
struct xxc_thread           __xxc_main_thread;

static struct {
    const char *image;
    size_t      filesz;
    size_t      memsz;
    size_t      align;
    size_t      off;
} g_tls = { NULL, 0, 0, 1, 0 };

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

    const xxc_phdr_t *ph = NULL;
    size_t phnum = 0;
    for (; av[0] != AT_NULL; av += 2)
    {
        if (av[0] == AT_PHDR)  ph = (const xxc_phdr_t *)av[1];
        if (av[0] == AT_PHNUM) phnum = (size_t)av[1];
    }
    if (!ph) return;

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
#if defined(__x86_64__)
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

void __xxc_linux_init(int argc, char **argv)
{
    if (has_interp(argc, argv))
    {
        struct xxc_thread *t = &__xxc_main_thread;
        t->self = t;
        atomic_i32_init(&t->tid, (i32)sys_gettid());
        atomic_i32_init(&t->state, XXC_THREAD_DETACHED);
        __xxc_self = t;
        return;
    }
    tls_scan(argc, argv);

    size_t len = align_up_u(__xxc_tls_area_size() + 64, 4096);
    char *area = (char *)sys_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (!area)
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
