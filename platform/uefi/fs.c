// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"
#include <mem.h>

static EFI_FILE_PROTOCOL *g_root;

static EFI_FILE_PROTOCOL *root(void)
{
    if (g_root) return g_root;

    EFI_GUID lig = EFI_LOADED_IMAGE_GUID, sfg = EFI_SIMPLE_FILE_SYSTEM_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *li = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = NULL;
    EFI_BOOT_SERVICES *bs = __uefi_st->BootServices;

    if (bs->HandleProtocol(__uefi_image, &lig, (void **)&li) != EFI_SUCCESS || !li) return NULL;
    if (bs->HandleProtocol(li->DeviceHandle, &sfg, (void **)&sfs) != EFI_SUCCESS || !sfs) return NULL;
    if (sfs->OpenVolume(sfs, &g_root) != EFI_SUCCESS) g_root = NULL;
    return g_root;
}

#define PATH_MAX_ 512
static bool to_ucs2(const char *path, CHAR16 *out)
{
    size_t i = 0;
    for (; path[i]; i++)
    {
        if (i >= PATH_MAX_ - 1 || (unsigned char)path[i] >= 0x80) return false;
        out[i] = (path[i] == '/') ? L'\\' : (CHAR16)(unsigned char)path[i];
    }
    out[i] = 0;
    return i > 0;
}

static EFI_FILE_PROTOCOL *open_raw(const char *path, UINT64 mode)
{
    CHAR16 wpath[PATH_MAX_];
    EFI_FILE_PROTOCOL *r = root(), *f = NULL;
    if (!r || !to_ucs2(path, wpath)) return NULL;
    if (r->Open(r, &f, wpath, mode, 0) != EFI_SUCCESS) return NULL;
    return f;
}

void *__uefi_fopen(const char *path, const char *mode)
{
    bool plus = mode[1] == '+';
    EFI_FILE_PROTOCOL *f = NULL;

    switch (mode[0])
    {
        case 'r':
            return open_raw(path, plus ? (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE) : EFI_FILE_MODE_READ);

        case 'w':
            f = open_raw(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE);
            if (f) f->Delete(f);
            return open_raw(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE);

        case 'a':
            f = open_raw(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE);
            if (f) f->SetPosition(f, ~0ULL);
            return f;
    }
    return NULL;
}

i64 __uefi_fread(void *h, void *buf, size_t n)
{
    if (UEFI_IS_STD(h)) return h == UEFI_STDIN ? __uefi_read_console(buf, n) : -1;
    EFI_FILE_PROTOCOL *f = h;
    UINTN len = n;
    if (f->Read(f, &len, buf) != EFI_SUCCESS) return -1;
    return (i64)len;
}

i64 __uefi_fwrite(void *h, const void *buf, size_t n)
{
    if (UEFI_IS_STD(h))
    {
        if (h == UEFI_STDIN) return -1;
        __uefi_write(buf, n, h == UEFI_STDERR);
        return (i64)n;
    }
    EFI_FILE_PROTOCOL *f = h;
    UINTN len = n;
    if (f->Write(f, &len, (void *)buf) != EFI_SUCCESS) return -1;
    return (i64)len;
}

i64 __uefi_ftell(void *h)
{
    if (UEFI_IS_STD(h)) return -1;
    EFI_FILE_PROTOCOL *f = h;
    UINT64 pos;
    return f->GetPosition(f, &pos) == EFI_SUCCESS ? (i64)pos : -1;
}

i64 __uefi_fseek(void *h, i64 off, int whence)
{
    if (UEFI_IS_STD(h)) return -1;
    EFI_FILE_PROTOCOL *f = h;
    UINT64 base = 0;

    if (whence == 1) { if (f->GetPosition(f, &base) != EFI_SUCCESS) return -1; }
    else if (whence == 2)
    {
        if (f->SetPosition(f, ~0ULL) != EFI_SUCCESS) return -1;
        if (f->GetPosition(f, &base) != EFI_SUCCESS) return -1;
    }
    else if (whence != 0) return -1;

    i64 target = (i64)base + off;
    if (target < 0) return -1;
    if (f->SetPosition(f, (UINT64)target) != EFI_SUCCESS) return -1;
    return target;
}

void __uefi_fclose(void *h)
{
    if (!h || UEFI_IS_STD(h)) return;
    ((EFI_FILE_PROTOCOL *)h)->Close(h);
}

bool __uefi_remove(const char *path)
{
    EFI_FILE_PROTOCOL *f = open_raw(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE);
    if (!f) return false;
    return f->Delete(f) == EFI_SUCCESS;
}

bool __uefi_touch(const char *path)
{
    EFI_FILE_PROTOCOL *f = open_raw(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE);
    if (!f) return false;
    f->Close(f);
    return true;
}