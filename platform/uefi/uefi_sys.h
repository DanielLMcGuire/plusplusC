// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef UEFI_SYS_H
#define UEFI_SYS_H

#include <def.h>
#include "uefi.h"

extern EFI_SYSTEM_TABLE *__uefi_st;
extern EFI_HANDLE        __uefi_image;

void  *__uefi_malloc(size_t size);
void  *__uefi_calloc(size_t count, size_t size);
void  *__uefi_realloc(void *ptr, size_t size);
void   __uefi_free(void *ptr);
size_t __uefi_usable_size(void *ptr);

void   __uefi_write(const char *s, size_t n, unsigned int stream);   /* stream: 0 = out, 1 = err */
int    __uefi_getchar(void);
i64    __uefi_read_console(char *buf, size_t n);

#define UEFI_STDIN   ((void *)1)
#define UEFI_STDOUT  ((void *)2)
#define UEFI_STDERR  ((void *)3)
#define UEFI_IS_STD(h) ((uintptr_t)(h) <= 3)

void  *__uefi_fopen(const char *path, const char *mode);
i64    __uefi_fread(void *h, void *buf, size_t n);
i64    __uefi_fwrite(void *h, const void *buf, size_t n);
i64    __uefi_fseek(void *h, i64 off, int whence);
i64    __uefi_ftell(void *h);
void   __uefi_fclose(void *h);
bool   __uefi_remove(const char *path);
bool   __uefi_touch(const char *path);

void   __uefi_clock_init(void);
void   __uefi_stall_us(u64 us);
bool   __uefi_get_time(EFI_TIME *t);
u64    __uefi_unix_time(void);

void   __uefi_halt(int status) XXC_NORETURN;

#endif