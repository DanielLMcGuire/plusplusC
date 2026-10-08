// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <cio.h>
#include <def.h>
#include <str.h>

#include "format/str-format.inl"
#include "format/int-format.inl"
#include "format/float-format.inl"

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args) 
{
    size_t idx = 0;
    
    while (*fmt) 
    {
        if (*fmt != '%') 
        {
            emit_char(buf, size, &idx, *fmt++);
            continue;
        }
        fmt++;
        
        int zero_pad = 0, left_align = 0, plus_sign = 0, space_sign = 0, alt_form = 0;

        while (1) 
        {
            if (*fmt == '-') left_align = 1;
            else if (*fmt == '+') plus_sign = 1;
            else if (*fmt == ' ') space_sign = 1;
            else if (*fmt == '#') alt_form = 1;
            else if (*fmt == '0') zero_pad = 1;
            else break;
            fmt++;
        }

        if (left_align) zero_pad = 0;
        if (plus_sign) space_sign = 0;
        
        int width = 0;
        if (*fmt == '*') 
        {
            width = va_arg(args, int);
            if (width < 0) 
            {
                left_align = 1;
                width = -width;
            }
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') 
            {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        int precision = -1;
        if (*fmt == '.') 
        {
            fmt++;
            if (*fmt == '*') 
            {
                precision = va_arg(args, int);
                if (precision < 0) precision = -1;
                fmt++;
            } else {
                precision = 0;
                while (*fmt >= '0' && *fmt <= '9') 
                {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }
        
        int is_char = 0, is_short = 0, is_long = 0, is_long_long = 0;
        int is_size_t = 0, is_intmax_t = 0, is_ptrdiff_t = 0, is_long_double = 0;
        
        bool proc = true;
        while (proc) 
        {
            switch (*fmt)
            {
                case 'h':
                {
                    fmt++;
                    if (*fmt == 'h')
                    {
                        is_char = 1;
                        fmt++;
                    }
                    else 
                    { 
                        is_short = 1; 
                    }
                    break;
                }
                case 'l':
                {
                    fmt++;
                    if (*fmt == 'l')
                    { 
                        is_long_long = 1; fmt++; 
                    }
                    else 
                    { 
                        is_long = 1; 
                    }
                    break;
                }
                case 'z':
                {
                    is_size_t = 1;
                    fmt++;
                    break;
                }
                case 'j':
                {
                    is_intmax_t = 1;
                    fmt++;
                    break;
                }
                case 't': 
                {
                    is_ptrdiff_t = 1;
                    fmt++;
                    break;
                }
                case 'L':
                {
                    is_long_double = 1;
                    fmt++;
                    break;
                }
                default: proc = false;
            }
        }

        switch (*fmt)
        {
            case 'c':
            {
                int pad = width - 1;
                if (!left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                emit_char(buf, size, &idx, (char)va_arg(args, int));
                if (left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                break;
            }
            case 's':
            {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                int slen = 0;
                while (s[slen] != '\0' && (precision < 0 || slen < precision))
                    slen++;
                
                int pad = width - slen;
                if (!left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                for (int k = 0; k < slen; k++) emit_char(buf, size, &idx, s[k]);
                if (left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                break;
            }
            case 'd':
            case 'i':
            {
                long long sval;
                if (is_intmax_t) sval = (long long)va_arg(args, intmax_t);
                else if (is_ptrdiff_t) sval = (long long)va_arg(args, ptrdiff_t);
                else if (is_size_t) sval = (long long)va_arg(args, ssize_t);
                else if (is_long_long) sval = va_arg(args, long long);
                else if (is_long) sval = (long long)va_arg(args, long);
                else if (is_char) sval = (long long)(signed char)va_arg(args, int);
                else if (is_short) sval = (long long)(short)va_arg(args, int);
                else sval = (long long)va_arg(args, int);

                int is_negative = (sval < 0);
                unsigned long long uval = is_negative ? (unsigned long long)0 - (unsigned long long)sval : (unsigned long long)sval;
                emit_int(buf, size, &idx, uval, is_negative, 10, 0, width, zero_pad, left_align, plus_sign, space_sign, alt_form, precision);
                break;
            }
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            {
                unsigned long long uval;
                if (is_intmax_t) uval = (unsigned long long)va_arg(args, uintmax_t);
                else if (is_ptrdiff_t) uval = (unsigned long long)va_arg(args, size_t);
                else if (is_size_t) uval = (unsigned long long)va_arg(args, size_t);
                else if (is_long_long) uval = va_arg(args, unsigned long long);
                else if (is_long) uval = (unsigned long long)va_arg(args, unsigned long);
                else if (is_char) uval = (unsigned long long)(unsigned char)va_arg(args, int);
                else if (is_short) uval = (unsigned long long)(unsigned short)va_arg(args, int);
                else uval = (unsigned long long)va_arg(args, unsigned int);

                int base = (*fmt == 'o') ? 8 : (*fmt == 'u') ? 10 : 16;
                int uppercase = (*fmt == 'X');
                emit_int(buf, size, &idx, uval, 0, base, uppercase, width, zero_pad, left_align, plus_sign, space_sign, alt_form, precision);
                break;
            }
            case 'p':
            {
                void *ptr = va_arg(args, void *);
                if (!ptr)
                {
                    const char *nil_str = "(nil)";
                    int slen = 5;
                    int pad = width - slen;
                    if (!left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                    for (int k = 0; k < slen; k++) emit_char(buf, size, &idx, nil_str[k]);
                    if (left_align) while (pad-- > 0) emit_char(buf, size, &idx, ' ');
                } 
                else 
                {
                    emit_int(buf, size, &idx, (unsigned long long)(uintptr_t)ptr, 0, 16, 0, width, 0, left_align, plus_sign, space_sign, 1, -1);
                }
                break;
            }
            case 'f':
            case 'F': 
            {
                long double fval = is_long_double ? va_arg(args, long double) : (long double)va_arg(args, double);
                emit_float(buf, size, &idx, fval, width, zero_pad, precision, *fmt == 'F', left_align, plus_sign, space_sign, alt_form);
                break;
            }
            case 'e':
            case 'E': 
            {
                long double fval = is_long_double ? va_arg(args, long double) : (long double)va_arg(args, double);
                emit_float_sci(buf, size, &idx, fval, width, zero_pad, precision, *fmt == 'E', left_align, plus_sign, space_sign, alt_form);
                break;
            }
            case 'g':
            case 'G': 
            {
                long double fval = is_long_double ? va_arg(args, long double) : (long double)va_arg(args, double);
                emit_float_g(buf, size, &idx, fval, width, zero_pad, precision, *fmt == 'G', left_align, plus_sign, space_sign, alt_form);
                break;
            }
            case 'a':
            case 'A': 
            {
                long double fval = is_long_double ? va_arg(args, long double) : (long double)va_arg(args, double);
                emit_float_hex(buf, size, &idx, fval, width, zero_pad, precision, *fmt == 'A', left_align, plus_sign, space_sign, alt_form, is_long_double);
                break;
            }
            case 'n':
                if (is_intmax_t) 
                {
                    intmax_t *p = va_arg(args, intmax_t *);
                    if (p) *p = (intmax_t)idx;
                } 
                else if (is_ptrdiff_t) 
                {
                    ptrdiff_t *p = va_arg(args, ptrdiff_t *);
                    if (p) *p = (ptrdiff_t)idx;
                } 
                else if (is_size_t) 
                {
                    ssize_t *p = va_arg(args, ssize_t *);
                    if (p) *p = (ssize_t)idx;
                } 
                else if (is_long_long) 
                {
                    long long *p = va_arg(args, long long *);
                    if (p) *p = (long long)idx;
                } 
                else if (is_long) 
                {
                    long *p = va_arg(args, long *);
                    if (p) *p = (long)idx;
                } 
                else if (is_char) 
                {
                    signed char *p = va_arg(args, signed char *);
                    if (p) *p = (signed char)idx;
                } 
                else if (is_short) 
                {
                    short *p = va_arg(args, short *);
                    if (p) *p = (short)idx;
                } 
                else 
                {
                    int *p = va_arg(args, int *);
                    if (p) *p = (int)idx;
                }
                break;
            case '%':
                emit_char(buf, size, &idx, '%');
                break;
            default:
                emit_char(buf, size, &idx, '%');
                if (*fmt) emit_char(buf, size, &idx, *fmt);
                break;
        }
        if (*fmt) fmt++;
    }

    if (buf && size > 0) 
    {
        if (idx < size) 
            buf[idx] = '\0';
        else
            buf[size - 1] = '\0';
    }

    return (int)idx;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}