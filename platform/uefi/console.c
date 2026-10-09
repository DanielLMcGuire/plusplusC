
// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"

void __uefi_write(const char *s, size_t n, unsigned int stream)
{
    EFI_SIMPLE_TEXT_OUTPUT *out = stream ? __uefi_st->StdErr : __uefi_st->ConOut;
    if (out == nullptr || n == nullptr) return;

    CHAR16 buf[130];
    size_t j = 0;
    for (size_t i = 0; i < n; i++)
    {
        unsigned char c = (unsigned char)s[i];
        if (c == '\n') buf[j++] = L'\r';
        buf[j++] = (c < 0x80) ? c : '?';
        if (j >= 126 || i + 1 == n)
        {
            buf[j] = 0;
            out->OutputString(out, buf);
            j = 0;
        }
    }
}

int __uefi_getchar(void)
{
    EFI_SIMPLE_TEXT_INPUT *in = __uefi_st->ConIn;
    if (in == nullptr) return -1;

    for (;;)
    {
        EFI_INPUT_KEY key;
        EFI_STATUS st = in->ReadKeyStroke(in, &key);
        if (st == EFI_SUCCESS)
        {
            if (key.UnicodeChar) return key.UnicodeChar < 0x80 ? (int)key.UnicodeChar : '?';
            continue;
        }
        if (st != EFI_NOT_READY) return -1;

        UINTN idx;
        __uefi_st->BootServices->WaitForEvent(1, &in->WaitForKey, &idx);
    }
}

#define LINE_MAX_ 512
static char  g_line[LINE_MAX_];
static size_t g_line_len, g_line_pos;
static bool   g_line_eof;

i64 __uefi_read_console(char *buf, size_t n)
{
    if (g_line_pos >= g_line_len)
    {
        g_line_len = g_line_pos = 0;
        g_line_eof = false;

        for (;;)
        {
            int c = __uefi_getchar();
            if (c < 0) return -1;

            if (c == '\r' || c == '\n')
            {
                g_line[g_line_len++] = '\n';
                __uefi_write("\n", 1, 0);
                break;
            }
            if (c == 0x04 && g_line_len == 0) return 0;
            if (c == 0x08 || c == 0x7F)
            {
                if (g_line_len > 0) { g_line_len--; __uefi_write("\b \b", 3, 0); }
                continue;
            }
            if (c < 0x20) continue;
            if (g_line_len < LINE_MAX_ - 1)
            {
                g_line[g_line_len++] = (char)c;
                char ch = (char)c;
                __uefi_write(&ch, 1, 0);
            }
        }
    }

    size_t k = 0;
    while (k < n && g_line_pos < g_line_len) buf[k++] = g_line[g_line_pos++];
    return (i64)k;
}
