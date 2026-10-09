// ++C
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <mem.h>
#include <parr.h>
#include <dstr.h>
#include <str.h>
#include <alloc.h>

parr_t parr_new(const void **init, size_t init_len)
{
    parr_t arr = {0, 0, 0};
    
    size_t cap = init_len < 15 ? 15 : init_len * 2; 
    arr.data = (void**)malloc(cap * sizeof(void*));
    
    if (arr.data)
    {
        if (init && init_len > 0)
            memcpy(arr.data, init, init_len * sizeof(void*));
        arr.len = init ? init_len : 0;
        arr.cap = cap;
    }
    return arr;
}

void parr_append(parr_t *arr, const void **append, size_t append_len)
{
    if (arr == nullptr || arr->data == nullptr || append == nullptr || append_len == 0) return;
    
    size_t new_len = arr->len + append_len;
    if (new_len > arr->cap)
    {
        size_t new_cap = new_len * 2;
        void **new_data = (void**)malloc(new_cap * sizeof(void*));
        if (new_data == nullptr) return;
        
        memcpy(new_data, arr->data, arr->len * sizeof(void*));
        free(arr->data);
        arr->data = new_data;
        arr->cap = new_cap;
    }
    
    memcpy(arr->data + arr->len, append, append_len * sizeof(void*));
    arr->len = new_len;
}

void parr_append_val(parr_t *arr, void *val)
{
    if (arr == nullptr || arr->data == nullptr) return;
    
    size_t new_len = arr->len + 1;
    if (new_len > arr->cap)
    {
        size_t new_cap = new_len * 2;
        void **new_data = (void**)malloc(new_cap * sizeof(void*));
        if (new_data == nullptr) return;
        
        memcpy(new_data, arr->data, arr->len * sizeof(void*));
        free(arr->data);
        arr->data = new_data;
        arr->cap = new_cap;
    }
    
    arr->data[arr->len] = val;
    arr->len = new_len;
}

void parr_append_cstr(parr_t *arr, const char *str)
{
    if (arr == nullptr) return;

    size_t len = str ? strlen(str) : 0;
    char *copy = (char*)malloc(len + 1);
    if (copy == nullptr) return;

    if (str && len > 0)
    {
        memcpy(copy, str, len);
    }
    copy[len] = '\0';

    parr_append_val(arr, copy);
}

void parr_append_dstr(parr_t *arr, const dstr_t *ds) 
{
    if (arr == nullptr) return;

    dstr_t *item = (dstr_t*)malloc(sizeof(dstr_t));
    if (item == nullptr) return;

    const char *initial = (ds && ds->data) ? ds->data : "";
    *item = dstr_new(initial);

    parr_append_val(arr, item);
}

parr_t parr_c_strings_to_dstr(const parr_t *csArr) 
{
    if (csArr == nullptr) 
    {
        parr_t empty = {0};
        return empty;
    }

    parr_t dsArr = parr_new(nullptr, 0);

    for (size_t i = 0; i < csArr->len; i++) 
    {
        const char *cstr = (const char*)csArr->data[i];

        dstr_t *item = (dstr_t*)malloc(sizeof(dstr_t));
        if (item) 
        {
            *item = dstr_new(cstr);
            parr_append_val(&dsArr, item);
        }
    }

    return dsArr;
}

parr_t parr_dstr_to_c_strings(const parr_t *dsArr)
{
    if (dsArr == nullptr) 
    {
        parr_t empty = {0};
        return empty;
    }

    parr_t csArr = parr_new(nullptr, 0);

    for (size_t i = 0; i < dsArr->len; i++) 
    {
        const dstr_t *item = (const dstr_t*)dsArr->data[i];
        const char *src = (item && item->data) ? item->data : "";
        
        parr_append_cstr(&csArr, src);
    }

    return csArr;
}

void parr_free(parr_t *arr) 
{
    if (arr != nullptr && arr->data != nullptr) 
    {
        free(arr->data);
        arr->data = 0;
        arr->len = arr->cap = 0;
    }
}

void parr_free_c_strings(parr_t *csArr) 
{
    if (csArr == nullptr || csArr->data ) return;
    for (size_t i = 0; i < csArr->len; i++) 
        if (csArr->data[i] != nullptr)
            free(csArr->data[i]);
    parr_free(csArr);
}

void parr_free_dstr(parr_t *dsArr)
{
    if (dsArr == nullptr || dsArr->data == nullptr) return;
    for (size_t i = 0; i < dsArr->len; i++)
    {
        dstr_t *item = (dstr_t*)dsArr->data[i];
        if (item != nullptr)
        {
            dstr_free(item);
            free(item);
        }
    }
    parr_free(dsArr);
}