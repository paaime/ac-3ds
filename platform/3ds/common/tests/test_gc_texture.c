/* Host tests for gc_texture.c. */
#include "gc_texture.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define RGBA(r, g, b, a) \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

static void put_be16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void test_single_entry_decoders(void)
{
    CHECK(gc_decode_rgb5a3(0xFFFF) == RGBA(255, 255, 255, 255)); /* RGB555 white */
    CHECK(gc_decode_rgb5a3(0x8000) == RGBA(0, 0, 0, 255));       /* RGB555 black */
    CHECK(gc_decode_rgb5a3(0xFC00) == RGBA(255, 0, 0, 255));     /* RGB555 red */
    CHECK(gc_decode_rgb5a3(0x7FFF) == RGBA(255, 255, 255, 255)); /* ARGB3444 a=7 */
    CHECK(gc_decode_rgb5a3(0x0000) == RGBA(0, 0, 0, 0));         /* transparent */
    CHECK(gc_decode_rgb5a3(0x3F00) == RGBA(255, 0, 0, 109));     /* a=3 -> 3*255/7 */
    CHECK(gc_decode_rgb565(0xF800) == RGBA(255, 0, 0, 255));
    CHECK(gc_decode_rgb565(0x07E0) == RGBA(0, 255, 0, 255));
    CHECK(gc_decode_rgb565(0x001F) == RGBA(0, 0, 255, 255));
}

static void test_sizes(void)
{
    CHECK(gc_texture_size(GC_TF_C4, 32, 48) == 0x300); /* matches the mailbox asset */
    CHECK(gc_texture_size(GC_TF_C4, 32, 16) == 0x100);
    CHECK(gc_texture_size(GC_TF_I8, 8, 4) == 32);
    CHECK(gc_texture_size(GC_TF_IA8, 4, 4) == 32);
    CHECK(gc_texture_size(GC_TF_RGBA8, 4, 4) == 64);
    CHECK(gc_texture_size(GC_TF_C4, 4, 4) == 32); /* padded to one 8x8 block */
    CHECK(gc_texture_size(GC_TF_CMPR, 8, 8) == 32);
    CHECK(gc_texture_size((GcTexFormat)0x7, 8, 8) == 0);
}

/* 16-entry RGB5A3 palette: entry i = distinct opaque colours. */
static void make_palette(uint16_t* entries, bool big_endian)
{
    for (int i = 0; i < 16; i++) {
        uint16_t value = (uint16_t)(0x8000 | (i << 10) | ((15 - i) << 5) | (i & 7));
        if (big_endian) {
            put_be16((uint8_t*)&entries[i], value);
        } else {
            entries[i] = value;
        }
    }
}

static uint32_t palette_colour(int i)
{
    return gc_decode_rgb5a3((uint16_t)(0x8000 | (i << 10) | ((15 - i) << 5) | (i & 7)));
}

static void test_ci4_block_layout(bool big_endian_palette)
{
    /* 16x8 = two 8x8 blocks side by side. Texel index = (x + y) & 15. */
    uint8_t src[64];
    for (int block = 0; block < 2; block++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x += 2) {
                int gx = block * 8 + x;
                int hi = (gx + y) & 15;
                int lo = (gx + 1 + y) & 15;
                src[block * 32 + y * 4 + x / 2] = (uint8_t)((hi << 4) | lo); /* high nibble = left */
            }
        }
    }
    uint16_t entries[16];
    make_palette(entries, big_endian_palette);
    GcPalette palette = { entries, GC_TLUT_RGB5A3, 16, big_endian_palette };
    uint32_t dst[16 * 8];

    CHECK(gc_texture_decode(dst, src, 16, 8, GC_TF_C4, &palette));
    int wrong = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 16; x++) {
            wrong += dst[y * 16 + x] != palette_colour((x + y) & 15);
        }
    }
    CHECK(wrong == 0);
}

static void test_ci4_partial_block_and_missing_entries(void)
{
    /* 4x8 image still occupies a full 8x8 block; indices above the palette
     * size fall back to the grey ramp (entry 15 -> grey 15). */
    uint8_t src[32];
    memset(src, 0xFF, sizeof(src));
    uint16_t entries[2] = { 0x8000, 0xFFFF };
    GcPalette palette = { entries, GC_TLUT_RGB5A3, 2, false };
    uint32_t dst[4 * 8];

    CHECK(gc_texture_decode(dst, src, 4, 8, GC_TF_C4, &palette));
    CHECK(dst[0] == RGBA(15, 15, 15, 255));
    CHECK(dst[4 * 8 - 1] == RGBA(15, 15, 15, 255));
}

static void test_ci8(void)
{
    uint8_t src[32]; /* one 8x4 block */
    for (int i = 0; i < 32; i++) {
        src[i] = (uint8_t)i;
    }
    uint16_t entries[32];
    for (int i = 0; i < 32; i++) {
        entries[i] = (uint16_t)(0x8000 | i);
    }
    GcPalette palette = { entries, GC_TLUT_RGB5A3, 32, false };
    uint32_t dst[8 * 4];

    CHECK(gc_texture_decode(dst, src, 8, 4, GC_TF_C8, &palette));
    CHECK(dst[0] == gc_decode_rgb5a3(0x8000));
    CHECK(dst[3 * 8 + 7] == gc_decode_rgb5a3(0x8000 | 31));
}

static void test_ia8_palette_byte_order(void)
{
    /* IA8 palette entries: intensity in the first byte, alpha in the second,
     * whichever endianness flag is set (the PC port's LE path reads the same
     * bytes: "LE read of BE bytes [I,A]"). */
    uint8_t bytes[2] = { 0x80, 0x40 };
    uint8_t src[32] = { 0 };
    uint32_t dst_be[8 * 8], dst_le[8 * 8];
    GcPalette p_be = { bytes, GC_TLUT_IA8, 1, true };
    GcPalette p_le = { bytes, GC_TLUT_IA8, 1, false };

    CHECK(gc_texture_decode(dst_be, src, 8, 8, GC_TF_C4, &p_be));
    CHECK(gc_texture_decode(dst_le, src, 8, 8, GC_TF_C4, &p_le));
    CHECK(dst_be[0] == RGBA(0x80, 0x80, 0x80, 0x40));
    CHECK(dst_le[0] == RGBA(0x80, 0x80, 0x80, 0x40));
}

static void test_other_palette_paths(void)
{
    uint8_t src[32] = { 0x01 }; /* texel 0 -> index 0, texel 1 -> index 1 */
    uint32_t dst[8 * 8];

    /* RGB565 palette (big-endian). */
    uint8_t rgb565[4] = { 0xF8, 0x00, 0x07, 0xE0 };
    GcPalette p565 = { rgb565, GC_TLUT_RGB565, 2, true };
    CHECK(gc_texture_decode(dst, src, 8, 8, GC_TF_C4, &p565));
    CHECK(dst[0] == RGBA(255, 0, 0, 255));
    CHECK(dst[1] == RGBA(0, 255, 0, 255));

    /* No palette data: the grey ramp (index i -> grey i). */
    GcPalette empty = { NULL, GC_TLUT_RGB5A3, 16, false };
    CHECK(gc_texture_decode(dst, src, 8, 8, GC_TF_C4, &empty));
    CHECK(dst[1] == RGBA(1, 1, 1, 255));

    /* More than 256 entries are clamped, not read past the table. */
    static uint16_t big[GC_PALETTE_MAX_ENTRIES];
    for (int i = 0; i < GC_PALETTE_MAX_ENTRIES; i++) {
        big[i] = 0xFFFF;
    }
    GcPalette oversized = { big, GC_TLUT_RGB5A3, 1000, false };
    CHECK(gc_texture_decode(dst, src, 8, 8, GC_TF_C4, &oversized));
    CHECK(dst[0] == RGBA(255, 255, 255, 255));
}

static void test_intensity_formats(void)
{
    uint8_t i4[32] = { 0xF0 };  /* texel 0 = 15, texel 1 = 0 */
    uint8_t i8[32] = { 0x7B };
    uint8_t ia4[32] = { 0x5A }; /* alpha 5, intensity 10 */
    uint8_t ia8[32] = { 0x40, 0xC0 }; /* alpha byte, then intensity byte */
    uint32_t d8x8[8 * 8], d8x4[8 * 4], d4x4[4 * 4];

    CHECK(gc_texture_decode(d8x8, i4, 8, 8, GC_TF_I4, NULL));
    CHECK(d8x8[0] == RGBA(0xFF, 0xFF, 0xFF, 0xFF));
    CHECK(d8x8[1] == RGBA(0, 0, 0, 0));
    CHECK(gc_texture_decode(d8x4, i8, 8, 4, GC_TF_I8, NULL));
    CHECK(d8x4[0] == RGBA(0x7B, 0x7B, 0x7B, 0x7B));
    CHECK(gc_texture_decode(d8x4, ia4, 8, 4, GC_TF_IA4, NULL));
    CHECK(d8x4[0] == RGBA(0xAA, 0xAA, 0xAA, 0x55));
    CHECK(gc_texture_decode(d4x4, ia8, 4, 4, GC_TF_IA8, NULL));
    CHECK(d4x4[0] == RGBA(0xC0, 0xC0, 0xC0, 0x40));
}

static void test_16bit_block_layout(void)
{
    /* 8x4 = two 4x4 blocks; texel value encodes its position. */
    uint8_t src[64];
    for (int block = 0; block < 2; block++) {
        for (int i = 0; i < 16; i++) {
            int x = block * 4 + i % 4;
            int y = i / 4;
            put_be16(&src[block * 32 + i * 2], (uint16_t)(0x8000 | (x << 5) | y));
        }
    }
    uint32_t dst[8 * 4];
    CHECK(gc_texture_decode(dst, src, 8, 4, GC_TF_RGB5A3, NULL));
    int wrong = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 8; x++) {
            wrong += dst[y * 8 + x] != gc_decode_rgb5a3((uint16_t)(0x8000 | (x << 5) | y));
        }
    }
    CHECK(wrong == 0);

    uint8_t rgb565[32];
    put_be16(rgb565, 0xF800);
    CHECK(gc_texture_decode(dst, rgb565, 4, 4, GC_TF_RGB565, NULL));
    CHECK(dst[0] == RGBA(255, 0, 0, 255));
}

static void test_rgba8_two_planes(void)
{
    uint8_t src[64] = { 0 };
    src[0] = 0x11; /* texel 0 alpha */
    src[1] = 0x22; /* texel 0 red */
    src[32] = 0x33; /* texel 0 green */
    src[33] = 0x44; /* texel 0 blue */
    uint32_t dst[16];

    CHECK(gc_texture_decode(dst, src, 4, 4, GC_TF_RGBA8, NULL));
    CHECK(dst[0] == RGBA(0x22, 0x33, 0x44, 0x11));
}

static void test_cmpr(void)
{
    /* Sub-block 0: c0 > c1 (opaque 4-colour); sub-block 1: c0 <= c1 (index 3 transparent). */
    uint8_t src[32] = { 0 };
    put_be16(&src[0], 0xF800); /* red */
    put_be16(&src[2], 0x001F); /* blue */
    src[4] = 0x1B;             /* row 0 indices 0,1,2,3 */
    put_be16(&src[8], 0x001F);
    put_be16(&src[10], 0xF800);
    src[12] = 0xFF;            /* row 0 all index 3 */
    uint32_t dst[8 * 8];

    CHECK(gc_texture_decode(dst, src, 8, 8, GC_TF_CMPR, NULL));
    CHECK(dst[0] == RGBA(255, 0, 0, 255));
    CHECK(dst[1] == RGBA(0, 0, 255, 255));
    CHECK(dst[2] == RGBA(170, 0, 85, 255)); /* (2*c0 + c1) / 3 */
    CHECK(dst[4] == RGBA(0, 0, 0, 0));      /* sub-block 1, x = 4: transparent */
}

static void test_rejects_invalid_arguments(void)
{
    uint8_t src[32] = { 0 };
    uint32_t dst[64];
    dst[0] = 0xDEADBEEF;

    CHECK(!gc_texture_decode(dst, src, 8, 8, GC_TF_C4, NULL));        /* CI without palette */
    CHECK(!gc_texture_decode(dst, src, 8, 8, (GcTexFormat)0x7, NULL)); /* unknown format */
    CHECK(!gc_texture_decode(dst, src, 0, 8, GC_TF_I8, NULL));
    CHECK(!gc_texture_decode(NULL, src, 8, 8, GC_TF_I8, NULL));
    CHECK(!gc_texture_decode(dst, NULL, 8, 8, GC_TF_I8, NULL));
    CHECK(dst[0] == 0xDEADBEEF);
}

int main(void)
{
    test_single_entry_decoders();
    test_sizes();
    test_ci4_block_layout(false);
    test_ci4_block_layout(true);
    test_ci4_partial_block_and_missing_entries();
    test_ci8();
    test_ia8_palette_byte_order();
    test_other_palette_paths();
    test_intensity_formats();
    test_16bit_block_layout();
    test_rgba8_two_planes();
    test_cmpr();
    test_rejects_invalid_arguments();

    if (failures != 0) {
        fprintf(stderr, "gc_texture: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gc_texture: all tests passed\n");
    return 0;
}
