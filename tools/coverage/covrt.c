// ++C Runtime Library
// Licensed under the MIT License

#include <stddef.h>
#include <parr.h>

typedef unsigned int u32;
typedef long long s64;

#define GCOV_COUNTERS 8
#define TAG_FUNCTION 0x01000000u
#define TAG_COUNTER_BASE 0x01a10000u
#define TAG_OBJECT_SUMMARY 0xa1000000u
#define GCDA_MAGIC 0x67636461u

struct gcov_ctr_info { u32 num; s64 *values; };

struct gcov_fn_info
{
    struct gcov_info *key;
    u32 ident;
    u32 lineno_checksum;
    u32 cfg_checksum;
    struct gcov_ctr_info ctrs[1];
};

struct gcov_info
{
    u32 version;
    struct gcov_info *next;
    u32 stamp;
    u32 checksum;
    const char *filename;
    void (*merge[GCOV_COUNTERS])(void);
    u32 n_functions;
    struct gcov_fn_info **functions;
};

static struct gcov_info *g_list;

void __gcov_init(struct gcov_info *info)
{
    if (!info || *(const u32 *)info == 0)
        return;

    for (struct gcov_info *p = g_list; p; p = p->next)
        if (p == info)
            return;

    info->next = g_list;
    g_list = info;
}

void __gcov_exit(void) {}
void __gcov_merge_add(s64 *c, u32 n) { (void)c; (void)n; }

static long sys3(long n, long a, long b, long c)
{
    long r;
    __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c)
                     : "rcx", "r11", "memory");
    return r;
}

static long sys_open(const char *p, long flags, long mode) { return sys3(2, (long)p, flags, mode); }
static long sys_read(long fd, void *b, long n)             { return sys3(0, fd, (long)b, n); }
static long sys_write(long fd, const void *b, long n)      { return sys3(1, fd, (long)b, n); }
static void sys_close(long fd)                             { sys3(3, fd, 0, 0); }

#define BUF_WORDS (1u << 20)
static u32 g_wbuf[BUF_WORDS];
static u32 g_rbuf[BUF_WORDS];

static u32 load_previous(const char *filename,
                         u32 version,
                         u32 stamp,
                         u32 checksum,
                         u32 *dst_words,
                         u32 *dst_n_words)
{
    long fd = sys_open(filename, 0 /* O_RDONLY */, 0);
    if (fd < 0)
        return 0;

    long total = 0;

    while (total < (long)sizeof(g_rbuf))
    {
        long got = sys_read(fd,
                            (char *)g_rbuf + total,
                            (long)sizeof(g_rbuf) - total);
        if (got <= 0)
            break;
        total += got;
    }

    sys_close(fd);

    if (total < 0 || (total % 4) != 0)
        return 0;

    u32 words = (u32)(total / 4);

    if (words < 8 ||
        g_rbuf[0] != GCDA_MAGIC ||
        g_rbuf[1] != version ||
        g_rbuf[2] != stamp ||
        g_rbuf[3] != checksum)
        return 0;

    if (g_rbuf[4] != TAG_OBJECT_SUMMARY ||
        g_rbuf[5] != 8)
        return 0;

    for (u32 i = 0; i < words; i++)
        dst_words[i] = g_rbuf[i];

    *dst_n_words = words;
    return words;
}

static void dump_one(const struct gcov_info *gi)
{
#if defined(__GNUC__) && !defined(__clang__)
# if __GNUC__ >= 15
    enum { COV_COUNTERS_LOCAL = 10 };
# elif __GNUC__ >= 14
    enum { COV_COUNTERS_LOCAL = 9 };
# elif __GNUC__ >= 10
    enum { COV_COUNTERS_LOCAL = 8 };
# else
    enum { COV_COUNTERS_LOCAL = 9 };
# endif
#else
    enum { COV_COUNTERS_LOCAL = 8 };
#endif

    struct gcov_info_view
    {
        u32 version;
        struct gcov_info *next;
        u32 stamp;
        u32 checksum;
        const char *filename;
        void (*merge[COV_COUNTERS_LOCAL])(s64 *, u32);
        u32 n_functions;
        const struct gcov_fn_info *const *functions;
    };

    const struct gcov_info_view *vgi =
        (const struct gcov_info_view *)(const void *)gi;

    if (!vgi->filename || !vgi->functions || !vgi->n_functions)
        return;

    int slots[COV_COUNTERS_LOCAL];
    int nslots = 0;

    for (int i = 0; i < COV_COUNTERS_LOCAL; i++)
        if (vgi->merge[i])
            slots[nslots++] = i;

    static u32 prev[BUF_WORDS];

    u32 prev_words = 0;
    u32 prev_pos = 4;
    u32 runs = 0;
    u32 prev_sum_max = 0;

    int have_prev =
        load_previous(vgi->filename,
                      vgi->version,
                      vgi->stamp,
                      vgi->checksum,
                      prev,
                      &prev_words) != 0;

    if (have_prev)
    {
        if (prev_pos + 4 <= prev_words &&
            prev[prev_pos] == TAG_OBJECT_SUMMARY &&
            prev[prev_pos + 1] == 8)
        {
            runs = prev[prev_pos + 2];
            prev_sum_max = prev[prev_pos + 3];
            prev_pos += 4;
        }
        else
        {
            have_prev = 0;
        }
    }

    u32 w = 0;

    g_wbuf[w++] = GCDA_MAGIC;
    g_wbuf[w++] = vgi->version;
    g_wbuf[w++] = vgi->stamp;
    g_wbuf[w++] = vgi->checksum;

    u32 summary_at = w;

    g_wbuf[w++] = TAG_OBJECT_SUMMARY;
    g_wbuf[w++] = 8;
    g_wbuf[w++] = 0;
    g_wbuf[w++] = 0;

    unsigned long long run_max = 0;

    for (u32 f = 0; f < vgi->n_functions; f++)
    {
        const struct gcov_fn_info *fi = vgi->functions[f];

        if (!fi)
            continue;

        if (fi->key != gi)
            continue;

        g_wbuf[w++] = TAG_FUNCTION;
        g_wbuf[w++] = 12;
        g_wbuf[w++] = fi->ident;
        g_wbuf[w++] = fi->lineno_checksum;
        g_wbuf[w++] = fi->cfg_checksum;

        if (have_prev)
        {
            if (prev_pos + 5 <= prev_words &&
                prev[prev_pos] == TAG_FUNCTION &&
                prev[prev_pos + 1] == 12 &&
                prev[prev_pos + 2] == fi->ident &&
                prev[prev_pos + 3] == fi->lineno_checksum &&
                prev[prev_pos + 4] == fi->cfg_checksum)
            {
                prev_pos += 5;
            }
            else
            {
                have_prev = 0;
            }
        }

        const struct gcov_ctr_info *ci = fi->ctrs;

        for (int c = 0; c < nslots; c++, ci++)
        {
            u32 counter_tag =
                TAG_COUNTER_BASE + ((u32)slots[c] << 17);

            u32 counter_bytes = ci->num * 2u * 4u;

            g_wbuf[w++] = counter_tag;
            g_wbuf[w++] = counter_bytes;

            const u32 *pv = 0;

            if (have_prev)
            {
                if (prev_pos + 2 + ci->num * 2 <= prev_words &&
                    prev[prev_pos] == counter_tag &&
                    prev[prev_pos + 1] == counter_bytes)
                {
                    pv = &prev[prev_pos + 2];
                    prev_pos += 2 + ci->num * 2;
                }
                else
                {
                    have_prev = 0;
                }
            }

            for (u32 i = 0; i < ci->num; i++)
            {
                unsigned long long current =
                    (unsigned long long)ci->values[i];

                if (slots[c] == 0 && current > run_max)
                    run_max = current;

                unsigned long long value = current;

                if (pv)
                {
                    unsigned long long old =
                        ((unsigned long long)pv[i * 2 + 1] << 32) |
                        (unsigned long long)pv[i * 2];

                    value += old;
                }

                g_wbuf[w++] = (u32)value;
                g_wbuf[w++] = (u32)(value >> 32);
            }
        }
    }

    g_wbuf[summary_at + 2] = runs + 1;
    g_wbuf[summary_at + 3] =
        (u32)((unsigned long long)prev_sum_max + run_max);

    long fd =
        sys_open(vgi->filename,
                 01 | 0100 | 01000 /* WRONLY|CREAT|TRUNC */,
                 0644);

    if (fd < 0)
        return;

    long bytes = (long)w * 4;
    long off = 0;

    while (off < bytes)
    {
        long n = sys_write(fd,
                           (char *)g_wbuf + off,
                           bytes - off);
        if (n <= 0)
            break;
        off += n;
    }

    sys_close(fd);
}

static void cov_dump(void)
{
    for (struct gcov_info *gi = g_list; gi; gi = gi->next)
        dump_one(gi);
}

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern int __real_program(parr_t args);
extern void __real_sys_exit(int status);

int __wrap_program(parr_t args)
{
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++)
        (*f)();
    return __real_program(args);
}

void __wrap_sys_exit(int status)
{
    cov_dump();
    __real_sys_exit(status);
    for (;;) {}
}