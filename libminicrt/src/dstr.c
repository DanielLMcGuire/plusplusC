// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <mem.h>
#include <dstr.h>
#include <str.h>
#include <alloc.h>

dstr_t dstr_new(const char *init)
{
    dstr_t s = {0, 0, 0};
    size_t len = init ? strlen(init) : 0;
    size_t cap = len < 15 ? 15 : len * 2; 
    s.data = (char*)malloc(cap + 1);
    if (s.data != nullptr)
    {
        if (init) memcpy(s.data, init, len);
        s.data[len] = '\0';
        s.len = len;
        s.cap = cap;
    }
    return s;
}

void dstr_append(dstr_t *s, const char *append)
{
    if (s == nullptr || s->data == nullptr || append == nullptr) return;
    size_t app_len = strlen(append);
    size_t new_len = s->len + app_len;
    if (new_len > s->cap)
    {
        size_t new_cap = new_len * 2;
        char *new_data = (char*)malloc(new_cap + 1);
        if (new_data == nullptr) return;
        memcpy(new_data, s->data, s->len);
        free(s->data);
        s->data = new_data;
        s->cap = new_cap;
    }
    memcpy(s->data + s->len, append, app_len);
    s->data[new_len] = '\0';
    s->len = new_len;
}

void dstr_append_char(dstr_t *s, char append)
{
    if (s == nullptr || s->data == nullptr) return;
    size_t new_len = s->len + 1;
    if (new_len > s->cap)
    {
        size_t new_cap = new_len * 2;
        char *new_data = (char*)malloc(new_cap + 1);
        if (new_data == nullptr) return;
        memcpy(new_data, s->data, s->len);
        free(s->data);
        s->data = new_data;
        s->cap = new_cap;
    }
    s->data[s->len] = append;
    s->data[new_len] = '\0';
    s->len = new_len;
}

void dstr_append_int(dstr_t *s, int val)
{
    if (s == nullptr || s->data == nullptr) return;
    if (val == 0)
    {
        dstr_append_char(s, '0');
        return;
    }
    unsigned int uval;
    if (val < 0)
    {
        dstr_append_char(s, '-');
        uval = -(unsigned int)val; 
    } else {
        uval = (unsigned int)val;
    }
    char buf[32];
    int i = 0;
    while (uval > 0)
    {
        buf[i++] = (char)('0' + (uval % 10));
        uval /= 10;
    }
    while (i > 0)
    {
        i--;
        dstr_append_char(s, buf[i]);
    }
}

void dstr_append_size_t(dstr_t *s, size_t val)
{
    if (s == nullptr || s->data == nullptr) return;
    if (val == 0)
    {
        dstr_append_char(s, '0');
        return;
    }
    char buf[32];
    int i = 0;
    while (val > 0)
    {
        buf[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    while (i > 0)
    {
        i--;
        dstr_append_char(s, buf[i]);
    }
}

void dstr_free(dstr_t *s) 
{
    if (s != nullptr && s->data != nullptr) 
    {
        free(s->data);
        s->data = 0;
        s->len = s->cap = 0;
    }
}