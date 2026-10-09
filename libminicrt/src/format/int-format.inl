// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

static unsigned int udiv64_small(unsigned long long *uval, unsigned int base)
{
    unsigned long long quotient = 0;
    unsigned int remainder = 0;

    for (int bit = 63; bit >= 0; bit--)
    {
        remainder <<= 1;
        remainder |= (unsigned int)((*uval >> bit) & 1ULL);
        if (remainder >= base)
        {
            remainder -= base;
            quotient |= ((unsigned long long)1 << bit);
        }
    }

    *uval = quotient;
    return remainder;
}

static void emit_int(char *buf, size_t size, size_t *idx, 
                     unsigned long long uval, int is_negative, int base, int uppercase, 
                     int width, int zero_pad, int left_align, 
                     int plus_sign, int space_sign, int alt_form, int precision) 
{
    char temp[70];
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    bool is_nonzero = (uval != 0);

    if (is_nonzero || precision != 0)
    {
        if (!is_nonzero)
        {
            temp[i++] = '0';
        }
        else
        {
            while (uval > 0)
            {
                unsigned int digit = udiv64_small(&uval, (unsigned int)base);
                temp[i++] = digits[digit];
            }
        }
    }

    char sign_char = 0;
    if (is_negative)
    {
        sign_char = '-';
    } 
    else 
    {
        if (plus_sign) sign_char = '+';
        else if (space_sign) sign_char = ' ';
    }

    char prefix1 = 0, prefix2 = 0;
    if (alt_form)
    {
        if (base == 8)
        {
            if (i == 0 || temp[i - 1] != '0')
                temp[i++] = '0';
        } 
        else if (base == 16 && is_nonzero)
        {
            prefix1 = '0';
            prefix2 = uppercase ? 'X' : 'x';
        }
    }

    int zero_pad_len = 0;
    if (precision >= 0)
    {
        zero_pad_len = precision - i;
        if (zero_pad_len < 0) zero_pad_len = 0;
    }
    else if (zero_pad && !left_align)
    {
        int prefix_len = (prefix1 ? 1 : 0) + (prefix2 ? 1 : 0);
        int sign_len = (sign_char ? 1 : 0);
        zero_pad_len = width - (i + prefix_len + sign_len);
        if (zero_pad_len < 0) zero_pad_len = 0;
    }

    int prefix_len = (prefix1 ? 1 : 0) + (prefix2 ? 1 : 0);
    int sign_len = (sign_char ? 1 : 0);
    int content_len = i + zero_pad_len + prefix_len + sign_len;
    int pad_chars = width - content_len;

    if (!left_align)
        while (pad_chars-- > 0) emit_char(buf, size, idx, ' ');

    
    if (sign_char)
        emit_char(buf, size, idx, sign_char);

    
    if (prefix1) emit_char(buf, size, idx, prefix1);
    if (prefix2) emit_char(buf, size, idx, prefix2);

    while (zero_pad_len-- > 0)
        emit_char(buf, size, idx, '0');

    while (i > 0)
        emit_char(buf, size, idx, temp[--i]);

    if (left_align)
        while (pad_chars-- > 0) emit_char(buf, size, idx, ' ');
}