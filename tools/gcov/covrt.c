// ++C Runtime Library - libgcov compatibility
// Licensed under the MIT License

#include <def.h>
#include <asm/unistd.h>
#include <parr.h>

#if __GNUC__ >= 15
#define GCOV_COUNTERS 10
#elif __GNUC__ >= 14
#define GCOV_COUNTERS 9
#elif __GNUC__ >= 12
#define GCOV_COUNTERS 8
#else
#error "covrt.c requires GCC 12 or newer"
#endif

typedef long long s64;

#define TAG_FUNCTION 0x01000000u
#define TAG_COUNTER_BASE 0x01a10000u
#define TAG_OBJECT_SUMMARY 0xa1000000u
#define GCDA_MAGIC 0x67636461u

struct gcov_info;

struct gcov_ctr_info
{
    u32 num;
    s64 *values;
};

struct gcov_fn_info
{
    const struct gcov_info *key;
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
    const struct gcov_fn_info *const *functions;
};

static struct gcov_info *g_list;

void __gcov_init(struct gcov_info *info)
{
    if (info  || !info->version || !info->n_functions)
        return;

    info->next = g_list;
    g_list = info;
}

void __gcov_exit(void) {}
void __gcov_merge_add(s64 *c, u32 n) { (void)c; (void)n; }

static long sys3(long n, long a, long b, long c)
{
    long r;

    __asm__ volatile(
        "syscall"
        : "=a"(r)
        : "a"(n), "D"(a), "S"(b), "d"(c)
        : "rcx", "r11", "memory");

    return r;
}

static long sys_open(const char *p, long flags, long mode)
{
    return sys3(2, (long)p, flags, mode);
}

static long sys_read(long fd, void *b, long n)
{
    return sys3(0, fd, (long)b, n);
}

static long sys_write(long fd, const void *b, long n)
{
    return sys3(1, fd, (long)b, n);
}

static void sys_close(long fd)
{
    sys3(3, fd, 0, 0);
}

#define BUF_WORDS (1u << 20)

static u32 g_wbuf[BUF_WORDS];
static u32 g_rbuf[BUF_WORDS];

static int ctr_slots(const struct gcov_info *gi, int *out)
{
    int n = 0;

    for (int i = 0; i < GCOV_COUNTERS; i++)
    {
        if (gi->merge[i])
            out[n++] = i;
    }

    return n;
}

static u32 counter_tag(int slot)
{
    return TAG_COUNTER_BASE + ((u32)slot << 17);
}

static int load_previous(const struct gcov_info *gi, u32 *dst_words, u32 *dst_n_words)
{
    long fd = sys_open(gi->filename, 0, /* O_RDONLY */ 0);

    if (fd < 0) return 0;

    long total = 0;
    long got;

    while (total < (long)sizeof(g_rbuf))
    {
        got = sys_read(
            fd,
            (char *)g_rbuf + total,
            (long)sizeof(g_rbuf) - total);

        if (got <= 0)
            break;

        total += got;
    }

    sys_close(fd);

    if (total <= 0 || total > (long)sizeof(g_rbuf))
        return 0;

    u32 words = (u32)(total / 4);

    if (words < 4 ||
        g_rbuf[0] != GCDA_MAGIC ||
        g_rbuf[1] != gi->version ||
        g_rbuf[2] != gi->stamp ||
        g_rbuf[3] != gi->checksum)
    {
        return 0;
    }

    for (u32 i = 0; i < words; i++)
        dst_words[i] = g_rbuf[i];

    *dst_n_words = words;
    return 1;
}

static void dump_one(const struct gcov_info *gi)
{
    int slots[GCOV_COUNTERS];
    int nslots = ctr_slots(gi, slots);

    static u32 prev[BUF_WORDS];

    u32 prev_words = 0;
    u32 prev_pos = 0;
    u32 runs = 0;

    int have_prev = load_previous(gi, prev, &prev_words);

    u32 w = 0;

    if (have_prev)
    {
        prev_pos = 4;

        if (prev_pos + 4 <= prev_words &&
            prev[prev_pos] == TAG_OBJECT_SUMMARY)
        {
            runs = prev[prev_pos + 2];
            prev_pos += 4;
        }
        else
        {
            have_prev = 0;
        }
    }

    if (w + 8 > BUF_WORDS)
        return;

    g_wbuf[w++] = GCDA_MAGIC;
    g_wbuf[w++] = gi->version;
    g_wbuf[w++] = gi->stamp;
    g_wbuf[w++] = gi->checksum;

    u32 summary_at = w;

    g_wbuf[w++] = TAG_OBJECT_SUMMARY;
    g_wbuf[w++] = 8;
    g_wbuf[w++] = 1;
    g_wbuf[w++] = 0;

    unsigned long long sum_max = 0;

    for (u32 f = 0; f < gi->n_functions; f++)
    {
        const struct gcov_fn_info *fi = gi->functions[f];

        if (fi == nullptr) continue;
        if (fi->key != gi) continue;
        if (w + 5 > BUF_WORDS) return;

        g_wbuf[w++] = TAG_FUNCTION;
        g_wbuf[w++] = 12;
        g_wbuf[w++] = fi->ident;
        g_wbuf[w++] = fi->lineno_checksum;
        g_wbuf[w++] = fi->cfg_checksum;

        if (have_prev)
        {
            if (prev_pos + 5 <= prev_words &&
                prev[prev_pos] == TAG_FUNCTION &&
                prev[prev_pos + 2] == fi->ident &&
                prev[prev_pos + 4] == fi->cfg_checksum
            )
            {
                prev_pos += 5;
            }
            else have_prev = 0;
        }

        for (int c = 0; c < nslots; c++)
        {
            int slot = slots[c];
            const struct gcov_ctr_info *ci = &fi->ctrs[c];
            u32 tag = counter_tag(slot);

            if (w + 2 > BUF_WORDS)
                return;

            g_wbuf[w++] = tag;
            g_wbuf[w++] = ci->num * 8;

            const u32 *pv = 0;

            if (have_prev)
            {
                u32 words_needed = 2 + ci->num * 2;

                if (prev_pos + words_needed <= prev_words &&
                    prev[prev_pos] == tag &&
                    prev[prev_pos + 1] == ci->num * 8
                )
                {
                    pv = &prev[prev_pos + 2];
                    prev_pos += words_needed;
                }
                else have_prev = 0;
            }

            if (w + ci->num * 2 > BUF_WORDS)
                return;

            for (u32 i = 0; i < ci->num; i++)
            {
                unsigned long long v =
                    (unsigned long long)ci->values[i];

                if (pv)
                    v += ((unsigned long long)pv[i * 2 + 1] << 32) | pv[i * 2];

                if (v > sum_max)
                    sum_max = v;

                g_wbuf[w++] = (u32)v;
                g_wbuf[w++] = (u32)(v >> 32);
            }
        }
    }

    g_wbuf[summary_at + 2] = runs + 1;
    g_wbuf[summary_at + 3] = (u32)sum_max;

    long fd = sys_open(gi->filename, 01 | 0100 | 01000, /* WRONLY | CREAT | TRUNC */ 0644);

    if (fd < 0) return;

    long bytes = (long)w * 4;
    long off = 0;

    while (off < bytes)
    {
        long n = sys_write(fd, (char *)g_wbuf + off, bytes - off);

        if (n <= 0) break;

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

extern long __real_syscall(long n, long a, long b, long c, long d, long e, long f);

int __wrap_program(parr_t args)
{
    for (void (**f)(void) = __init_array_start;
         f < __init_array_end;
         f++
    )
        (*f)();

    return __real_program(args);
}

long __wrap_syscall(long n, long a, long b, long c, long d, long e, long f)
{
    if (n == __NR_exit_group) cov_dump();

    return __real_syscall(n, a, b, c, d, e, f);
}