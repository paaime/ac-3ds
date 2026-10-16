#include <stdio.h>
#include <string.h>

#include "../tga.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static unsigned u16le(const uint8_t* p)
{
    return (unsigned)(p[0] | (p[1] << 8));
}

static void test_size(void)
{
    CHECK(tga_rgba8_size(2, 3) == TGA_HEADER_BYTES + 2 * 3 * 4);
    CHECK(tga_rgba8_size(0, 3) == 0);
    CHECK(tga_rgba8_size(3, 0) == 0);
    CHECK(tga_rgba8_size(TGA_MAX_SIDE, TGA_MAX_SIDE) == TGA_HEADER_BYTES + (size_t)TGA_MAX_SIDE * TGA_MAX_SIDE * 4);
    CHECK(tga_rgba8_size(TGA_MAX_SIDE + 1, 1) == 0);
    CHECK(tga_rgba8_size(1, TGA_MAX_SIDE + 1) == 0);
    CHECK(tga_rgba8_size(32768, 32768) == 0);
}

static void test_header(void)
{
    uint32_t px[6] = { 0 };
    uint8_t out[TGA_HEADER_BYTES + 6 * 4];
    CHECK(tga_write_rgba8(out, sizeof(out), px, 3, 2) == sizeof(out));
    CHECK(out[0] == 0 && out[1] == 0);
    CHECK(out[2] == 2);
    CHECK(u16le(out + 12) == 3 && u16le(out + 14) == 2);
    CHECK(out[16] == 32);
    CHECK(out[17] == 0x28); /* 8 alpha bits, top-left origin */
}

static void test_pixels_in_bgra_order_top_row_first(void)
{
    uint32_t px[2] = { 0x11223344u, 0xAABBCCDDu }; /* 0xRRGGBBAA */
    uint8_t out[TGA_HEADER_BYTES + 2 * 4];
    CHECK(tga_write_rgba8(out, sizeof(out), px, 1, 2) == sizeof(out));
    const uint8_t* p = out + TGA_HEADER_BYTES;
    CHECK(p[0] == 0x33 && p[1] == 0x22 && p[2] == 0x11 && p[3] == 0x44);
    CHECK(p[4] == 0xCC && p[5] == 0xBB && p[6] == 0xAA && p[7] == 0xDD);
}

static void test_rejects_bad_arguments(void)
{
    uint32_t px[4] = { 0 };
    uint8_t out[TGA_HEADER_BYTES + 4 * 4];
    memset(out, 0x5A, sizeof(out));
    CHECK(tga_write_rgba8(out, sizeof(out) - 1, px, 2, 2) == 0);
    CHECK(out[0] == 0x5A);
    CHECK(tga_write_rgba8(NULL, sizeof(out), px, 2, 2) == 0);
    CHECK(tga_write_rgba8(out, sizeof(out), NULL, 2, 2) == 0);
    CHECK(tga_write_rgba8(out, sizeof(out), px, 0, 2) == 0);
}

int main(void)
{
    test_size();
    test_header();
    test_pixels_in_bgra_order_top_row_first();
    test_rejects_bad_arguments();
    if (failures != 0) {
        fprintf(stderr, "tga: %d check(s) failed\n", failures);
        return 1;
    }
    printf("tga: all tests passed\n");
    return 0;
}
