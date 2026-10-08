// ++C C Runtime Library (libminicrt) | Platform (UEFI)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include "../sys_uefi.h"
#include <parr.h>
#include <atexit.h>

extern int  program(parr_t csArgs);
extern void sio_init(unsigned int out, unsigned int err);
extern void xxc_io_init(void);

EFI_SYSTEM_TABLE *__uefi_st;
EFI_HANDLE        __uefi_image;

int _fltused = 0;

static const CHAR16 *uefi_get_image_filepath(const EFI_DEVICE_PATH_PROTOCOL *dp)
{
    while (dp)
    {
        UINT8 type    = dp->Type;
        UINT8 subtype = dp->SubType;
        UINT16 len    = (UINT16)(dp->Length[0] | (dp->Length[1] << 8));

        if (len < 4)
            break;

        if (type == 0x04 && subtype == 0x04)
            return (const CHAR16 *)(dp + 1);

        if (type == 0x7F)
            break;

        dp = (const EFI_DEVICE_PATH_PROTOCOL *)((const char *)dp + len);
    }
    return NULL;
}

static void ucs2_to_ascii(const CHAR16 *src, char *dst, size_t max)
{
    size_t i = 0;
    if (!src || !dst || max == 0) return;

    while (src[i] && i + 1 < max)
    {
        dst[i] = (src[i] < 0x80) ? (char)src[i] : '?';
        i++;
    }
    dst[i] = '\0';
}

void __uefi_halt(int status)
{
    if (status != 0)
    {
        char msg[] = "\nprogram exited with non-zero status\n";
        __uefi_write(msg, sizeof(msg) - 1, 1);
    }
    for (;;) __asm__ volatile("hlt");
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *st)
{
    __uefi_st    = st;
    __uefi_image = image;

    st->BootServices->SetWatchdogTimer(0, 0, 0, 0);

    sio_init(0, 0);
    __uefi_clock_init();
    xxc_io_init();

    char app_path[256] = "";
    EFI_GUID lig = EFI_LOADED_IMAGE_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *li = NULL;

    if (st->BootServices->HandleProtocol(image, &lig, (void **)&li) == EFI_SUCCESS && li && li->FilePath)
    {
        const CHAR16 *wpath = uefi_get_image_filepath(li->FilePath);
        if (wpath)
            ucs2_to_ascii(wpath, app_path, sizeof(app_path));
    }

    const char *argv[] = { app_path };
    parr_t args = parr_new((const void **)argv, 1);
    int ret = program(args);
    parr_free(&args);

    exit(ret);
    __NORETURN__ // GCOV_EXCL_LINE
}