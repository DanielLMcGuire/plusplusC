// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <mem.h>
#include <alloc.h>
#include <iarr.h>

iarr_t iarr_new(const int *init, size_t init_len)
{
    iarr_t arr = {0, 0, 0};
    size_t cap = init_len < 15 ? 15 : init_len * 2; 
    arr.data = (int*)malloc(cap * sizeof(int));
    if (arr.data != nullptr)
    {
        if (init && init_len > 0)
            memcpy(arr.data, init, init_len * sizeof(int));
        arr.len = init ? init_len : 0;
        arr.cap = cap;
    }
    return arr;
}

void iarr_append(iarr_t *arr, const int *append, size_t append_len)
{
    if (arr == nullptr || arr->data == nullptr || append == nullptr || append_len == 0) return;
    size_t new_len = arr->len + append_len;
    if (new_len > arr->cap)
    {
        size_t new_cap = new_len * 2;
        int *new_data = (int*)malloc(new_cap * sizeof(int));
        if (new_data == nullptr) return;
        memcpy(new_data, arr->data, arr->len * sizeof(int));
        free(arr->data);
        arr->data = new_data;
        arr->cap = new_cap;
    }
    memcpy(arr->data + arr->len, append, append_len * sizeof(int));
    arr->len = new_len;
}

void iarr_append_val(iarr_t *arr, int val)
{
    if (arr == nullptr || arr->data == nullptr) return;
    size_t new_len = arr->len + 1;
    if (new_len > arr->cap)
    {
        size_t new_cap = new_len * 2;
        int *new_data = (int*)malloc(new_cap * sizeof(int));
        if (new_data == nullptr) return;
        memcpy(new_data, arr->data, arr->len * sizeof(int));
        free(arr->data);
        arr->data = new_data;
        arr->cap = new_cap;
    }
    arr->data[arr->len] = val;
    arr->len = new_len;
}

void iarr_free(iarr_t *arr)
{
    if (arr != nullptr && arr->data != nullptr)
    {
        free(arr->data);
        arr->data = 0;
        arr->len = arr->cap = 0;
    }
}