#ifndef BMP_H
#define BMP_H

#include <def.h>
#include "display.pph"

#pragma pack(push, 1)
typedef struct {
    u16 bfType;
    u32 bfSize;
    u16 bfReserved1;
    u16 bfReserved2;
    u32 bfOffBits;
} bmp_file_header_t;

typedef struct {
    u32 biSize;
    i32 biWidth;
    i32 biHeight;
    u16 biPlanes; 
    u16 biBitCount;
    u32 biCompression;
    u32 biSizeImage;
    i32 biXPelsPerMeter;
    i32 biYPelsPerMeter;
    u32 biClrUsed;
    u32 biClrImportant;
} bmp_info_header_t;
#pragma pack(pop)

static bool draw_bitmap(Display *d, const u8 *data, size_t size, i32 dest_x, i32 dest_y)
{
    if (d == nullptr || data == nullptr || size < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t))
        return false;

    const bmp_file_header_t *file_hdr = (const bmp_file_header_t *)data;
    const bmp_info_header_t *info_hdr = (const bmp_info_header_t *)(data + sizeof(bmp_file_header_t));

    if (file_hdr->bfType != 0x4D42)
    {
        printf("draw_bitmap: Invalid BMP signature\n");
        return false;
    }

    if (info_hdr->biCompression != 0)
    {
        printf("draw_bitmap: Compressed BMPs not supported\n");
        return false;
    }

    if (info_hdr->biBitCount != 24 && info_hdr->biBitCount != 32)
    {
        printf("draw_bitmap: Only 24-bit and 32-bit BMPs supported\n");
        return false;
    }

    if (file_hdr->bfOffBits >= size)
        return false;

    u32 bpp = info_hdr->biBitCount / 8;
    i32 width = info_hdr->biWidth;
    bool bottom_up = info_hdr->biHeight > 0;
    i32 height = bottom_up ? info_hdr->biHeight : -info_hdr->biHeight;

    size_t row_stride = ((size_t)width * bpp + 3) & ~3;
    const u8 *pixels = data + file_hdr->bfOffBits;

    for (i32 y = 0; y < height; y++)
    {
        i32 screen_y = dest_y + y;
        if (screen_y < 0 || (u32)screen_y >= d->height)
            continue;

        i32 src_row = bottom_up ? (height - 1 - y) : y;
        const u8 *row_ptr = pixels + (size_t)src_row * row_stride;

        for (i32 x = 0; x < width; x++)
        {
            i32 screen_x = dest_x + x;
            if (screen_x < 0 || (u32)screen_x >= d->width)
                continue;

            const u8 *px = row_ptr + (size_t)x * bpp;
            u8 b = px[0];
            u8 g = px[1];
            u8 r = px[2];

            u32 c = CALL3(d, color, r, g, b);

            d->fb[(u64)screen_y * d->stride + screen_x] = c;
        }
    }

    return true;
}

#endif