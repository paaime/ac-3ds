/* Decoding rules ported from ACGC-PC-Port pc/src/pc_gx_texture.c (MIT, (c) 2026 FlyingMeta). */
#include "gc_texture.h"

#include <string.h>

#define BLOCK_BYTES 32

typedef struct {
    uint32_t r, g, b, a;
} Rgba;

static uint32_t pack(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

static uint32_t expand(uint32_t value, uint32_t max)
{
    return value * 255 / max;
}

static uint16_t read_be16(const uint8_t* p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

uint32_t gc_decode_rgb5a3(uint16_t value)
{
    if (value & 0x8000) { /* RGB555, opaque */
        return pack(expand((value >> 10) & 0x1F, 31), expand((value >> 5) & 0x1F, 31),
                    expand(value & 0x1F, 31), 255);
    }
    /* ARGB3444 */
    return pack(expand((value >> 8) & 0x0F, 15), expand((value >> 4) & 0x0F, 15),
                expand(value & 0x0F, 15), expand((value >> 12) & 0x07, 7));
}

uint32_t gc_decode_rgb565(uint16_t value)
{
    return pack(expand((value >> 11) & 0x1F, 31), expand((value >> 5) & 0x3F, 63),
                expand(value & 0x1F, 31), 255);
}

typedef struct {
    uint32_t block_w;
    uint32_t block_h;
    uint32_t block_bytes;
} BlockShape;

static bool block_shape(GcTexFormat format, BlockShape* shape)
{
    switch (format) {
    case GC_TF_I4:
    case GC_TF_C4:
    case GC_TF_CMPR:
        *shape = (BlockShape){ 8, 8, BLOCK_BYTES };
        return true;
    case GC_TF_I8:
    case GC_TF_IA4:
    case GC_TF_C8:
        *shape = (BlockShape){ 8, 4, BLOCK_BYTES };
        return true;
    case GC_TF_IA8:
    case GC_TF_RGB565:
    case GC_TF_RGB5A3:
        *shape = (BlockShape){ 4, 4, BLOCK_BYTES };
        return true;
    case GC_TF_RGBA8:
        *shape = (BlockShape){ 4, 4, 2 * BLOCK_BYTES };
        return true;
    default:
        return false;
    }
}

size_t gc_texture_size(GcTexFormat format, uint32_t width, uint32_t height)
{
    BlockShape shape;
    if (!block_shape(format, &shape)) {
        return 0;
    }
    size_t blocks_x = (width + shape.block_w - 1) / shape.block_w;
    size_t blocks_y = (height + shape.block_h - 1) / shape.block_h;
    return blocks_x * blocks_y * shape.block_bytes;
}

static void build_palette(const GcPalette* palette, uint32_t colours[GC_PALETTE_MAX_ENTRIES])
{
    for (uint32_t i = 0; i < GC_PALETTE_MAX_ENTRIES; i++) {
        colours[i] = pack(i, i, i, 255);
    }
    if (palette->data == NULL) {
        return;
    }
    uint32_t count = palette->count > GC_PALETTE_MAX_ENTRIES ? GC_PALETTE_MAX_ENTRIES : palette->count;
    const uint8_t* bytes = palette->data;
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t* entry = &bytes[i * 2];
        uint16_t value = palette->big_endian ? read_be16(entry) : (uint16_t)(entry[0] | (entry[1] << 8));
        switch (palette->format) {
        case GC_TLUT_RGB5A3:
            colours[i] = gc_decode_rgb5a3(value);
            break;
        case GC_TLUT_RGB565:
            colours[i] = gc_decode_rgb565(value);
            break;
        default: /* IA8: intensity in the first byte, alpha in the second */
            colours[i] = pack(entry[0], entry[0], entry[0], entry[1]);
            break;
        }
    }
}

typedef struct {
    uint32_t* dst;
    uint32_t width;
    uint32_t height;
    const uint32_t* palette;
} DecodeTarget;

static void put(const DecodeTarget* t, uint32_t x, uint32_t y, uint32_t texel)
{
    if (x < t->width && y < t->height) {
        t->dst[y * t->width + x] = texel;
    }
}

static uint32_t nibble_intensity(uint32_t n)
{
    return n | (n << 4);
}

typedef void (*BlockDecoder)(const DecodeTarget* t, const uint8_t* block, uint32_t x0, uint32_t y0);

static void block_i4(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 64; i++) {
        uint32_t n = (i & 1) ? (b[i / 2] & 0x0F) : (b[i / 2] >> 4);
        uint32_t v = nibble_intensity(n);
        put(t, x0 + i % 8, y0 + i / 8, pack(v, v, v, v));
    }
}

static void block_c4(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 64; i++) {
        uint32_t index = (i & 1) ? (b[i / 2] & 0x0F) : (b[i / 2] >> 4);
        put(t, x0 + i % 8, y0 + i / 8, t->palette[index]);
    }
}

static void block_i8(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 32; i++) {
        put(t, x0 + i % 8, y0 + i / 8, pack(b[i], b[i], b[i], b[i]));
    }
}

static void block_ia4(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 32; i++) {
        uint32_t a = nibble_intensity(b[i] >> 4);
        uint32_t v = nibble_intensity(b[i] & 0x0F);
        put(t, x0 + i % 8, y0 + i / 8, pack(v, v, v, a));
    }
}

static void block_c8(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 32; i++) {
        put(t, x0 + i % 8, y0 + i / 8, t->palette[b[i]]);
    }
}

static void block_ia8(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 16; i++) { /* alpha byte, then intensity byte */
        uint32_t a = b[i * 2];
        uint32_t v = b[i * 2 + 1];
        put(t, x0 + i % 4, y0 + i / 4, pack(v, v, v, a));
    }
}

static void block_rgb565(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 16; i++) {
        put(t, x0 + i % 4, y0 + i / 4, gc_decode_rgb565(read_be16(&b[i * 2])));
    }
}

static void block_rgb5a3(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t i = 0; i < 16; i++) {
        put(t, x0 + i % 4, y0 + i / 4, gc_decode_rgb5a3(read_be16(&b[i * 2])));
    }
}

static void block_rgba8(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    const uint8_t* ar = b;
    const uint8_t* gb = b + BLOCK_BYTES;
    for (uint32_t i = 0; i < 16; i++) {
        put(t, x0 + i % 4, y0 + i / 4, pack(ar[i * 2 + 1], gb[i * 2], gb[i * 2 + 1], ar[i * 2]));
    }
}

/* CMPR: 8x8 block of four 4x4 DXT1 sub-blocks (big-endian colours). */
static void block_cmpr(const DecodeTarget* t, const uint8_t* b, uint32_t x0, uint32_t y0)
{
    for (uint32_t sub = 0; sub < 4; sub++) {
        const uint8_t* s = &b[sub * 8];
        uint16_t c0 = read_be16(&s[0]);
        uint16_t c1 = read_be16(&s[2]);
        uint32_t e0 = gc_decode_rgb565(c0);
        uint32_t e1 = gc_decode_rgb565(c1);
        Rgba p0 = { e0 >> 24, (e0 >> 16) & 0xFF, (e0 >> 8) & 0xFF, 255 };
        Rgba p1 = { e1 >> 24, (e1 >> 16) & 0xFF, (e1 >> 8) & 0xFF, 255 };
        uint32_t colours[4] = { e0, e1, 0, 0 };
        if (c0 > c1) {
            colours[2] = pack((2 * p0.r + p1.r) / 3, (2 * p0.g + p1.g) / 3, (2 * p0.b + p1.b) / 3, 255);
            colours[3] = pack((p0.r + 2 * p1.r) / 3, (p0.g + 2 * p1.g) / 3, (p0.b + 2 * p1.b) / 3, 255);
        } else {
            colours[2] = pack((p0.r + p1.r) / 2, (p0.g + p1.g) / 2, (p0.b + p1.b) / 2, 255);
            colours[3] = 0;
        }
        uint32_t sx = x0 + (sub & 1) * 4;
        uint32_t sy = y0 + (sub >> 1) * 4;
        for (uint32_t y = 0; y < 4; y++) {
            for (uint32_t x = 0; x < 4; x++) {
                put(t, sx + x, sy + y, colours[(s[4 + y] >> (6 - x * 2)) & 3]);
            }
        }
    }
}

static BlockDecoder decoder_for(GcTexFormat format)
{
    switch (format) {
    case GC_TF_I4: return block_i4;
    case GC_TF_I8: return block_i8;
    case GC_TF_IA4: return block_ia4;
    case GC_TF_IA8: return block_ia8;
    case GC_TF_RGB565: return block_rgb565;
    case GC_TF_RGB5A3: return block_rgb5a3;
    case GC_TF_RGBA8: return block_rgba8;
    case GC_TF_C4: return block_c4;
    case GC_TF_C8: return block_c8;
    case GC_TF_CMPR: return block_cmpr;
    default: return NULL;
    }
}

bool gc_texture_decode(uint32_t* dst, const void* src, uint32_t width, uint32_t height,
                       GcTexFormat format, const GcPalette* palette)
{
    BlockShape shape;
    BlockDecoder decode = decoder_for(format);
    bool indexed = format == GC_TF_C4 || format == GC_TF_C8;
    if (dst == NULL || src == NULL || width == 0 || height == 0 || decode == NULL ||
        !block_shape(format, &shape) || (indexed && palette == NULL)) {
        return false;
    }

    uint32_t colours[GC_PALETTE_MAX_ENTRIES];
    if (indexed) {
        build_palette(palette, colours);
    }
    DecodeTarget target = { dst, width, height, indexed ? colours : NULL };
    memset(dst, 0, (size_t)width * height * sizeof(uint32_t));

    const uint8_t* block = src;
    for (uint32_t y0 = 0; y0 < height; y0 += shape.block_h) {
        for (uint32_t x0 = 0; x0 < width; x0 += shape.block_w) {
            decode(&target, block, x0, y0);
            block += shape.block_bytes;
        }
    }
    return true;
}
