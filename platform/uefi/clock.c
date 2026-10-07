// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"

static u64 g_tsc_hz;
static u64 g_tsc_base;

static inline u64 rdtsc(void)
{
    u32 lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;
}

void __uefi_stall_us(u64 us)
{
    while (us)
    {
        u64 c = us > 1000000 ? 1000000 : us;
        __uefi_st->BootServices->Stall(c);
        us -= c;
    }
}

void __uefi_clock_init(void)
{
    u64 best = 0;
    for (int i = 0; i < 3; i++)
    {
        u64 t0 = rdtsc();
        __uefi_st->BootServices->Stall(20000);
        u64 dt = rdtsc() - t0;
        if (best == 0 || dt < best) best = dt;
    }
    g_tsc_hz   = best * 50;
    g_tsc_base = rdtsc();
}

bool __uefi_get_time(EFI_TIME *t)
{
    return __uefi_st->RuntimeServices->GetTime(t, NULL) == EFI_SUCCESS;
}

static i64 days_from_civil(i64 y, unsigned m, unsigned d)
{
    y -= m <= 2;
    i64 era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (i64)doe - 719468;
}

u64 __uefi_unix_time(void)
{
    EFI_TIME t;
    if (!__uefi_get_time(&t)) return 0;
    i64 secs = days_from_civil(t.Year, t.Month, t.Day) * 86400 + t.Hour * 3600 + t.Minute * 60 + t.Second;
    if (t.TimeZone != EFI_UNSPECIFIED_TIMEZONE) secs += (i64)t.TimeZone * 60;
    return secs > 0 ? (u64)secs : 0;
}

u64 __uefi_monotonic_ns(void)
{
    if (!g_tsc_hz)
        return __uefi_unix_time() * 1000000000ull;

    u64 d    = rdtsc() - g_tsc_base;
    u64 secs = d / g_tsc_hz;
    u64 rem  = d % g_tsc_hz;
    return secs * 1000000000ull + (rem * 1000000000ull) / g_tsc_hz;
}