#include "tga.h"

#include <string.h>

#define TGA_TRUE_COLOUR  2
#define TGA_BITS         32
#define TGA_DESCRIPTOR   0x28 /* 8 alpha bits, rows from the top */
#define BYTES_PER_TEXEL  4u

size_t tga_rgba8_size(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > TGA_MAX_SIDE || height > TGA_MAX_SIDE) {
        return 0;
    }
    return TGA_HEADER_BYTES + (size_t)width * height * BYTES_PER_TEXEL;
}

static void put_u16le(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

size_t tga_write_rgba8(uint8_t* out, size_t out_size, const uint32_t* texels, uint32_t width, uint32_t height)
{
    size_t size = tga_rgba8_size(width, height);
    if (out == NULL || texels == NULL || size == 0 || out_size < size) {
        return 0;
    }
    memset(out, 0, TGA_HEADER_BYTES);
    out[2] = TGA_TRUE_COLOUR;
    put_u16le(out + 12, width);
    put_u16le(out + 14, height);
    out[16] = TGA_BITS;
    out[17] = TGA_DESCRIPTOR;
    uint8_t* p = out + TGA_HEADER_BYTES;
    for (size_t i = 0; i < (size_t)width * height; i++) {
        uint32_t c = texels[i]; /* 0xRRGGBBAA, written B, G, R, A */
        *p++ = (uint8_t)(c >> 8);
        *p++ = (uint8_t)(c >> 16);
        *p++ = (uint8_t)(c >> 24);
        *p++ = (uint8_t)c;
    }
    return size;
}
