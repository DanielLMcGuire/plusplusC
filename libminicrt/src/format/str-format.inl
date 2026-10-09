// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

static void emit_char(char *buf, size_t size, size_t *idx, char c)
{
    if (buf && size > 0 && *idx < size - 1) 
        buf[*idx] = c;

    (*idx)++;
}

static void emit_str(char *buf, size_t size, size_t *idx, const char *s)
{
    if (s == nullptr) s = "(null)";
    while (*s)
        emit_char(buf, size, idx, *s++);
}