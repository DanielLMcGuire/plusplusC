// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

static inline int ld_isnan(long double val)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_isnan(val);
#else
    return val != val;
#endif
}

static inline int ld_isinf(long double val)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_isinf(val);
#else
    return val != 0.0L && (val == val * 2.0L);
#endif
}

static inline int ld_signbit(long double val)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_signbit(val);
#else
    union { double d; unsigned long long u; } b;
    b.d = (double)val;
    return (int)(b.u >> 63);
#endif
}

#if defined(__LDBL_MANT_DIG__)
#define FP_LDBL_MANT   __LDBL_MANT_DIG__
#define FP_LDBL_MAXEXP __LDBL_MAX_EXP__
#define FP_LDBL_MINEXP __LDBL_MIN_EXP__
#else
#define FP_LDBL_MANT   53
#define FP_LDBL_MAXEXP 1024
#define FP_LDBL_MINEXP (-1021)
#endif

#define FP_IMAX (FP_LDBL_MAXEXP / 16 + 2)
#define FP_FMAX ((-FP_LDBL_MINEXP + FP_LDBL_MANT) / 16 + 3)
#define FP_IDIG ((FP_LDBL_MAXEXP * 30103) / 100000 + 8)
#define FP_FDIG (-FP_LDBL_MINEXP + FP_LDBL_MANT + 8)
#define FP_BUF  (FP_IDIG + FP_FDIG + 8)

typedef struct
{
    unsigned int ilimb[FP_IMAX];
    unsigned int flimb[FP_FMAX];
    int ni;
    int nf;
} fp_num;

static const unsigned int fp_pow10[] = { 1u, 10u, 100u, 1000u, 10000u };

static void fp_decode(long double v, fp_num *n)
{
    const long double B = 65536.0L;
    n->ni = 0;
    n->nf = 0;

    if (v >= 1.0L)
    {
        long double s = 1.0L;
        int k = 0;
        while (v >= s * B)
        {
            s *= B;
            k++;
        }
        n->ni = k + 1;
        for (int j = k; j >= 0; j--)
        {
            unsigned int d = (unsigned int)(v / s);
            n->ilimb[j] = d;
            v -= (long double)d * s;
            s /= B;
        }
    }

    while (v > 0.0L && n->nf < FP_FMAX)
    {
        v *= B;
        unsigned int d = (unsigned int)v;
        n->flimb[n->nf++] = d;
        v -= (long double)d;
    }
}

static int fp_int_digits(fp_num *n, char *out)
{
    if (n->ni == 0)
    {
        out[0] = '0';
        return 1;
    }

    char tmp[FP_IDIG + 8];
    int t = 0;
    int top = n->ni;
    unsigned int *a = n->ilimb;

    while (top > 0)
    {
        unsigned int rem = 0;
        for (int i = top - 1; i >= 0; i--)
        {
            unsigned int cur = (rem << 16) | a[i];
            a[i] = cur / 10000u;
            rem = cur % 10000u;
        }
        while (top > 0 && a[top - 1] == 0)
            top--;
        for (int j = 0; j < 4; j++)
        {
            tmp[t++] = (char)('0' + rem % 10);
            rem /= 10;
        }
    }

    while (t > 1 && tmp[t - 1] == '0')
        t--;
    for (int i = 0; i < t; i++)
        out[i] = tmp[t - 1 - i];
    return t;
}

static unsigned int fp_frac_take(fp_num *n, int k)
{
    unsigned int mul = fp_pow10[k];
    unsigned int carry = 0;

    for (int i = n->nf - 1; i >= 0; i--)
    {
        unsigned int cur = n->flimb[i] * mul + carry;
        n->flimb[i] = cur & 0xFFFFu;
        carry = cur >> 16;
    }
    while (n->nf > 0 && n->flimb[n->nf - 1] == 0)
        n->nf--;
    return carry;
}

static int fp_frac_state(const fp_num *n)
{
    if (n->nf == 0)
        return 0;
    if (n->flimb[0] > 0x8000u)
        return 3;
    if (n->flimb[0] < 0x8000u)
        return 1;
    return n->nf > 1 ? 3 : 2;
}

static int fp_gen_frac(fp_num *n, char *dst, int want)
{
    int made = 0;
    while (want > 0 && n->nf > 0)
    {
        int k = want < 4 ? want : 4;
        unsigned int c = fp_frac_take(n, k);
        for (int j = k - 1; j >= 0; j--)
        {
            dst[j] = (char)('0' + c % 10);
            c /= 10;
        }
        dst += k;
        made += k;
        want -= k;
    }
    return made;
}

static int fp_tail_state(const fp_num *n, const char *d, int nd)
{
    if (d[0] > '5')
        return 3;
    if (d[0] < '5')
        return 1;
    for (int i = 1; i < nd; i++)
        if (d[i] != '0')
            return 3;
    return fp_frac_state(n) != 0 ? 3 : 2;
}

static void fp_convert(long double v, int sci, int count, char *out, int *nout, int *exp10)
{
    fp_num num;
    fp_decode(v, &num);

    int n = 0;
    int rem = 0;
    int x = 0;

    if (!sci)
    {
        int ilen = fp_int_digits(&num, out);
        n = ilen + fp_gen_frac(&num, out + ilen, count);
        rem = fp_frac_state(&num);
        x = ilen;
    }
    else if (num.ni > 0)
    {
        int ilen = fp_int_digits(&num, out);
        x = ilen - 1;
        if (ilen > count)
        {
            rem = fp_tail_state(&num, out + count, ilen - count);
            n = count;
        }
        else
        {
            n = ilen + fp_gen_frac(&num, out + ilen, count - ilen);
            rem = fp_frac_state(&num);
        }
    }
    else
    {
        int lead = 0;
        unsigned int c;
        do
        {
            c = fp_frac_take(&num, 4);
            if (c == 0)
                lead += 4;
        } while (c == 0);

        char ch[4];
        for (int j = 3; j >= 0; j--)
        {
            ch[j] = (char)('0' + c % 10);
            c /= 10;
        }
        int z = 0;
        while (ch[z] == '0')
            z++;
        x = -(lead + z + 1);

        int avail = 4 - z;
        if (avail > count)
        {
            for (int i = 0; i < count; i++)
                out[i] = ch[z + i];
            rem = fp_tail_state(&num, ch + z + count, avail - count);
            n = count;
        }
        else
        {
            for (int i = 0; i < avail; i++)
                out[i] = ch[z + i];
            n = avail + fp_gen_frac(&num, out + avail, count - avail);
            rem = fp_frac_state(&num);
        }
    }

    if (rem == 3 || (rem == 2 && ((out[n - 1] - '0') & 1)))
    {
        int i = n - 1;
        while (i >= 0 && out[i] == '9')
        {
            out[i] = '0';
            i--;
        }
        if (i >= 0)
        {
            out[i]++;
        }
        else if (sci)
        {
            out[0] = '1';
            x++;
        }
        else
        {
            for (int j = n; j > 0; j--)
                out[j] = out[j - 1];
            out[0] = '1';
            n++;
            x++;
        }
    }

    *nout = n;
    *exp10 = x;
}

typedef struct
{
    const char *d;
    int n;
    int zero_int;
    int ia, ib;
    int lz;
    int fa, fb;
    int point;
    int has_exp;
    char expc;
    int exp;
} fp_layout;

static void emit_pad(char *buf, size_t size, size_t *idx, char c, int count)
{
    while (count-- > 0)
        emit_char(buf, size, idx, c);
}

static void emit_range(char *buf, size_t size, size_t *idx, const char *d, int n, int a, int b)
{
    for (int i = a; i < b; i++)
        emit_char(buf, size, idx, i < n ? d[i] : '0');
}

static int fp_exp_digits(int e, char *out)
{
    unsigned int u = (unsigned int)(e < 0 ? -e : e);
    char tmp[12];
    int t = 0;
    do
    {
        tmp[t++] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    for (int i = 0; i < t; i++)
        out[i] = tmp[t - 1 - i];
    return t;
}

static void emit_layout(char *buf, size_t size, size_t *idx, const fp_layout *l, char sign,
                        int width, int zero_pad, int left_align)
{
    char ed[12];
    int en = 0;
    int body = (l->zero_int ? 1 : l->ib - l->ia) + (l->point ? 1 : 0) + l->lz + (l->fb - l->fa);
    if (l->has_exp)
    {
        en = fp_exp_digits(l->exp, ed);
        body += 2 + (en < 2 ? 2 : en);
    }

    int pad = width - (sign ? 1 : 0) - body;
    if (!left_align && !zero_pad)
        emit_pad(buf, size, idx, ' ', pad);
    if (sign)
        emit_char(buf, size, idx, sign);
    if (!left_align && zero_pad)
        emit_pad(buf, size, idx, '0', pad);

    if (l->zero_int)
        emit_char(buf, size, idx, '0');
    else
        emit_range(buf, size, idx, l->d, l->n, l->ia, l->ib);
    if (l->point)
        emit_char(buf, size, idx, '.');
    emit_pad(buf, size, idx, '0', l->lz);
    emit_range(buf, size, idx, l->d, l->n, l->fa, l->fb);

    if (l->has_exp)
    {
        emit_char(buf, size, idx, l->expc);
        emit_char(buf, size, idx, l->exp < 0 ? '-' : '+');
        emit_pad(buf, size, idx, '0', 2 - en);
        for (int i = 0; i < en; i++)
            emit_char(buf, size, idx, ed[i]);
    }

    if (left_align)
        emit_pad(buf, size, idx, ' ', pad);
}

static char fp_sign_char(int negative, int plus_sign, int space_sign)
{
    return negative ? '-' : (plus_sign ? '+' : (space_sign ? ' ' : 0));
}

static int emit_nonfinite(char *buf, size_t size, size_t *idx, long double val, int width,
                          int uppercase, int left_align, int plus_sign, int space_sign)
{
    int nan = ld_isnan(val);
    if (!nan && !ld_isinf(val))
        return 0;

    char sign = fp_sign_char(ld_signbit(val), plus_sign, space_sign);
    const char *text = nan ? (uppercase ? "NAN" : "nan") : (uppercase ? "INF" : "inf");
    int pad = width - 3 - (sign ? 1 : 0);

    if (!left_align)
        emit_pad(buf, size, idx, ' ', pad);
    if (sign)
        emit_char(buf, size, idx, sign);
    emit_str(buf, size, idx, text);
    if (left_align)
        emit_pad(buf, size, idx, ' ', pad);
    return 1;
}

static void emit_float(char *buf, size_t size, size_t *idx,
                       long double val, int width, int zero_pad, int precision,
                       int uppercase, int left_align, int plus_sign, int space_sign, int alt_form)
{
    if (precision < 0)
        precision = 6;
    if (emit_nonfinite(buf, size, idx, val, width, uppercase, left_align, plus_sign, space_sign))
        return;

    int negative = ld_signbit(val);
    if (negative)
        val = -val;

    char digits[FP_BUF];
    int n, ilen;
    if (val == 0.0L)
    {
        digits[0] = '0';
        n = 1;
        ilen = 1;
    }
    else
    {
        fp_convert(val, 0, precision, digits, &n, &ilen);
    }

    fp_layout l = {0};
    l.d = digits;
    l.n = n;
    l.ia = 0;
    l.ib = ilen;
    l.fa = ilen;
    l.fb = ilen + precision;
    l.point = precision > 0 || alt_form;
    emit_layout(buf, size, idx, &l, fp_sign_char(negative, plus_sign, space_sign),
                width, zero_pad, left_align);
}

static void emit_float_sci(char *buf, size_t size, size_t *idx,
                           long double val, int width, int zero_pad, int precision,
                           int uppercase, int left_align, int plus_sign, int space_sign, int alt_form)
{
    if (precision < 0)
        precision = 6;
    if (emit_nonfinite(buf, size, idx, val, width, uppercase, left_align, plus_sign, space_sign))
        return;

    int negative = ld_signbit(val);
    if (negative)
        val = -val;

    char digits[FP_BUF];
    int n, x;
    if (val == 0.0L)
    {
        digits[0] = '0';
        n = 1;
        x = 0;
    }
    else
    {
        fp_convert(val, 1, precision + 1, digits, &n, &x);
    }

    fp_layout l = {0};
    l.d = digits;
    l.n = n;
    l.ia = 0;
    l.ib = 1;
    l.fa = 1;
    l.fb = precision + 1;
    l.point = precision > 0 || alt_form;
    l.has_exp = 1;
    l.expc = uppercase ? 'E' : 'e';
    l.exp = x;
    emit_layout(buf, size, idx, &l, fp_sign_char(negative, plus_sign, space_sign),
                width, zero_pad, left_align);
}

static void emit_float_g(char *buf, size_t size, size_t *idx,
                         long double val, int width, int zero_pad, int precision,
                         int uppercase, int left_align, int plus_sign, int space_sign, int alt_form)
{
    int P = precision < 0 ? 6 : (precision == 0 ? 1 : precision);
    if (emit_nonfinite(buf, size, idx, val, width, uppercase, left_align, plus_sign, space_sign))
        return;

    int negative = ld_signbit(val);
    if (negative)
        val = -val;

    char digits[FP_BUF];
    int n, x;
    if (val == 0.0L)
    {
        digits[0] = '0';
        n = 1;
        x = 0;
    }
    else
    {
        fp_convert(val, 1, P, digits, &n, &x);
    }

    int last = -1;
    for (int i = 0; i < n && i < P; i++)
        if (digits[i] != '0')
            last = i;

    fp_layout l = {0};
    l.d = digits;
    l.n = n;

    if (x < -4 || x >= P)
    {
        l.ia = 0;
        l.ib = 1;
        l.fa = 1;
        l.fb = alt_form ? P : (last + 1 > 1 ? last + 1 : 1);
        l.has_exp = 1;
        l.expc = uppercase ? 'E' : 'e';
        l.exp = x;
    }
    else if (x >= 0)
    {
        l.ia = 0;
        l.ib = x + 1;
        l.fa = x + 1;
        l.fb = alt_form ? P : (last + 1 > x + 1 ? last + 1 : x + 1);
    }
    else
    {
        l.zero_int = 1;
        l.lz = -x - 1;
        l.fa = 0;
        l.fb = alt_form ? P : last + 1;
    }
    l.point = (l.lz + (l.fb - l.fa)) > 0 || alt_form;

    emit_layout(buf, size, idx, &l, fp_sign_char(negative, plus_sign, space_sign),
                width, zero_pad, left_align);
}

#define FP_HEX_MAXNIB 28

static char fp_hex_digit(unsigned int v, int uppercase)
{
    return (char)(v < 10 ? '0' + v : (uppercase ? 'A' : 'a') + (v - 10));
}

static int fp_hex_split(long double val, int is_ld, unsigned int *lead, unsigned char *nib, int *exp2)
{
#if FP_LDBL_MANT == 64
    if (is_ld)
    {
        union { long double ld; struct { unsigned long long m; unsigned short se; } s; } u;
        u.ld = val;
        unsigned long long m = u.s.m;
        int be = u.s.se & 0x7fff;
        *exp2 = (be == 0 ? 1 : be) - 16383 - 3;
        *lead = (unsigned int)(m >> 60);
        for (int i = 0; i < 15; i++)
            nib[i] = (unsigned char)((m >> (56 - 4 * i)) & 0xf);
        return 15;
    }
#elif FP_LDBL_MANT == 113
    if (is_ld)
    {
        union { long double ld; struct { unsigned long long lo, hi; } s; } u;
        u.ld = val;
        int be = (int)((u.s.hi >> 48) & 0x7fff);
        *lead = be == 0 ? 0 : 1;
        *exp2 = (be == 0 ? 1 : be) - 16383;
        for (int i = 0; i < 12; i++)
            nib[i] = (unsigned char)((u.s.hi >> (44 - 4 * i)) & 0xf);
        for (int i = 0; i < 16; i++)
            nib[12 + i] = (unsigned char)((u.s.lo >> (60 - 4 * i)) & 0xf);
        return 28;
    }
#else
    (void)is_ld;
#endif
    union { double d; unsigned long long u; } b;
    b.d = (double)val;
    int be = (int)((b.u >> 52) & 0x7ff);
    *lead = be == 0 ? 0 : 1;
    *exp2 = (be == 0 ? 1 : be) - 1023;
    for (int i = 0; i < 13; i++)
        nib[i] = (unsigned char)((b.u >> (48 - 4 * i)) & 0xf);
    return 13;
}

static void emit_float_hex(char *buf, size_t size, size_t *idx,
                           long double val, int width, int zero_pad, int precision,
                           int uppercase, int left_align, int plus_sign, int space_sign, int alt_form,
                           int is_ld)
{
    if (emit_nonfinite(buf, size, idx, val, width, uppercase, left_align, plus_sign, space_sign))
        return;

    int negative = ld_signbit(val);
    if (negative)
        val = -val;

    unsigned int lead = 0;
    unsigned char nib[FP_HEX_MAXNIB];
    int nn = 0;
    int exp2 = 0;
    if (val != 0.0L)
        nn = fp_hex_split(val, is_ld, &lead, nib, &exp2);

    int shown;
    if (precision < 0)
    {
        shown = nn;
        while (shown > 0 && nib[shown - 1] == 0)
            shown--;
    }
    else
    {
        shown = precision;
        if (precision < nn)
        {
            unsigned int first = nib[precision];
            int rest = 0;
            for (int i = precision + 1; i < nn; i++)
                if (nib[i])
                    rest = 1;
            unsigned int last_kept = precision > 0 ? nib[precision - 1] : lead;
            if (first > 8 || (first == 8 && (rest || (last_kept & 1))))
            {
                int i = precision - 1;
                while (i >= 0 && nib[i] == 0xf)
                {
                    nib[i] = 0;
                    i--;
                }
                if (i >= 0)
                    nib[i]++;
                else
                    lead++;
            }
        }
    }

    if (lead >= 16)
    {
        lead >>= 4;
        exp2 += 4;
    }

    char ed[12];
    int en = fp_exp_digits(exp2, ed);
    int has_point = shown > 0 || alt_form;
    int body = 2 + 1 + (has_point ? 1 : 0) + shown + 2 + en;
    char sign = fp_sign_char(negative, plus_sign, space_sign);
    int pad = width - (sign ? 1 : 0) - body;

    if (!left_align && !zero_pad)
        emit_pad(buf, size, idx, ' ', pad);
    if (sign)
        emit_char(buf, size, idx, sign);
    emit_char(buf, size, idx, '0');
    emit_char(buf, size, idx, uppercase ? 'X' : 'x');
    if (!left_align && zero_pad)
        emit_pad(buf, size, idx, '0', pad);

    emit_char(buf, size, idx, fp_hex_digit(lead, uppercase));
    if (has_point)
        emit_char(buf, size, idx, '.');
    for (int i = 0; i < shown; i++)
        emit_char(buf, size, idx, i < nn ? fp_hex_digit(nib[i], uppercase) : '0');
    emit_char(buf, size, idx, uppercase ? 'P' : 'p');
    emit_char(buf, size, idx, exp2 < 0 ? '-' : '+');
    for (int i = 0; i < en; i++)
        emit_char(buf, size, idx, ed[i]);

    if (left_align)
        emit_pad(buf, size, idx, ' ', pad);
}
