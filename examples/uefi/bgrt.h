#ifndef BGRT_H
#define BGRT_H

#include <sys_uefi.h>
#include "display.pph"
#include "bmp.h"

#pragma pack(push, 1)
typedef struct {
    UINT32 Signature;
    UINT32 Length;
    UINT8  Revision;
    UINT8  Checksum;
    UINT8  OemId[6];
    UINT64 OemTableId;
    UINT32 OemRevision;
    UINT32 CreatorId;
    UINT32 CreatorRevision;
} ACPI_HEADER;

typedef struct {
    UINT8  Signature[8];
    UINT8  Checksum;
    UINT8  OemId[6];
    UINT8  Revision;
    UINT32 RsdtAddress;
    UINT32 Length;
    UINT64 XsdtAddress;
    UINT8  ExtendedChecksum;
    UINT8  Reserved[3];
} ACPI_RSDP;

typedef struct {
    ACPI_HEADER Header;
    UINT16 Version;
    UINT8  Status;
    UINT8  ImageType;
    UINT64 ImageAddress;
    UINT32 ImageOffsetX;
    UINT32 ImageOffsetY;
} ACPI_BGRT;
#pragma pack(pop)

static inline void draw_bgrt_logo(Display *disp)
{
    EFI_GUID acpi_guid = EFI_ACPI_20_TABLE_GUID;
    ACPI_RSDP *rsdp = nullptr;

    for (UINTN i = 0; i < __uefi_st->NumberOfTableEntries; i++)
    {
        if (memcmp(&__uefi_st->ConfigurationTable[i].VendorGuid, &acpi_guid, sizeof(EFI_GUID)) == 0)
        {
            rsdp = (ACPI_RSDP *)__uefi_st->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    if (rsdp == nullptr || rsdp->XsdtAddress == nullptr) return;

    ACPI_HEADER *xsdt = (ACPI_HEADER *)rsdp->XsdtAddress;
    int entries = (xsdt->Length - sizeof(ACPI_HEADER)) / sizeof(UINT64);
    UINT64 *table_ptrs = (UINT64 *)(xsdt + 1);

    ACPI_BGRT *bgrt = nullptr;

    for (int i = 0; i < entries; i++)
    {
        ACPI_HEADER *hdr = (ACPI_HEADER *)table_ptrs[i];
        if (memcmp(&hdr->Signature, "BGRT", 4) == 0)
        {
            bgrt = (ACPI_BGRT *)hdr;
            break;
        }
    }

    if (bgrt && (bgrt->Status & 1) && bgrt->ImageType == 0 && bgrt->ImageAddress)
    {
        bmp_file_header_t *bmp = (bmp_file_header_t *)bgrt->ImageAddress;
        
        if (bmp->bfType == 0x4D42)
        {
            draw_bitmap(disp, (const u8 *)bgrt->ImageAddress, bmp->bfSize, 
                        bgrt->ImageOffsetX, bgrt->ImageOffsetY);
        }
    }
}

#endif