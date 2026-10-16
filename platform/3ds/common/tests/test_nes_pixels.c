#include "nes_pixels.h"

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

static const uint8_t COLORS[4 * 3] = {
    0, 0, 0,
    255, 255, 255,
    255, 0, 0,
    0, 0, 255,
};

static void test_lut_maps_the_frames_range(void)
{
    int16_t palette[NES_PIXELS_LUT_SIZE] = { 0 };
    palette[64] = 1;
    palette[65] = 2;
    palette[66] = 3;
    palette[10] = 1;
    uint16_t lut[NES_PIXELS_LUT_SIZE];
    nes_pixels_lut(lut, palette, 64, 3, COLORS, 4);
    CHECK(lut[64] == 0xFFFF);
    CHECK(lut[65] == 0xF800);
    CHECK(lut[66] == 0x001F);
    CHECK(lut[10] == 0);
    CHECK(lut[63] == 0 && lut[67] == 0);
}

static void test_lut_rounds_down_to_565(void)
{
    const uint8_t grey[3] = { 0x84, 0x86, 0x88 }; /* 10000100 10000110 10001000 */
    int16_t palette[NES_PIXELS_LUT_SIZE] = { 0 };
    uint16_t lut[NES_PIXELS_LUT_SIZE];
    nes_pixels_lut(lut, palette, 0, 1, grey, 1);
    CHECK(lut[0] == ((0x10 << 11) | (0x21 << 5) | 0x11));
}

static void test_lut_rejects_colours_out_of_the_table(void)
{
    int16_t palette[NES_PIXELS_LUT_SIZE] = { 0 };
    palette[0] = 4;
    palette[1] = -1;
    palette[2] = 1;
    uint16_t lut[NES_PIXELS_LUT_SIZE];
    memset(lut, 0xAB, sizeof lut);
    nes_pixels_lut(lut, palette, 0, 3, COLORS, 4);
    CHECK(lut[0] == 0 && lut[1] == 0);
    CHECK(lut[2] == 0xFFFF);
}

static void test_lut_whole_range(void)
{
    int16_t palette[NES_PIXELS_LUT_SIZE];
    for (int i = 0; i < NES_PIXELS_LUT_SIZE; i++) {
        palette[i] = (int16_t)(i & 3);
    }
    uint16_t lut[NES_PIXELS_LUT_SIZE];
    nes_pixels_lut(lut, palette, 0, NES_PIXELS_LUT_SIZE, COLORS, 4);
    CHECK(lut[0] == 0 && lut[1] == 0xFFFF && lut[254] == 0xF800 && lut[255] == 0x001F);
}

static void test_converts_rows_with_a_pitch(void)
{
    uint16_t lut[NES_PIXELS_LUT_SIZE] = { 0 };
    lut[1] = 0x1111;
    lut[2] = 0x2222;
    lut[200] = 0xC8C8;
    const uint8_t src[10] = { 1, 2, 200, 9, 9, 2, 1, 1, 9, 9 };
    uint16_t dst[7];
    dst[6] = 0x7777;
    nes_pixels_to_rgb565(dst, src, 5, 3, 2, lut);
    CHECK(dst[0] == 0x1111 && dst[1] == 0x2222 && dst[2] == 0xC8C8);
    CHECK(dst[3] == 0x2222 && dst[4] == 0x1111 && dst[5] == 0x1111);
    CHECK(dst[6] == 0x7777);
}

static void test_no_rows_writes_nothing(void)
{
    uint16_t lut[NES_PIXELS_LUT_SIZE] = { 0 };
    const uint8_t src[1] = { 0 };
    uint16_t dst[1] = { 0x5555 };
    nes_pixels_to_rgb565(dst, src, 1, 1, 0, lut);
    CHECK(dst[0] == 0x5555);
}

int main(void)
{
    test_lut_maps_the_frames_range();
    test_lut_rounds_down_to_565();
    test_lut_rejects_colours_out_of_the_table();
    test_lut_whole_range();
    test_converts_rows_with_a_pitch();
    test_no_rows_writes_nothing();
    if (failures == 0) {
        printf("nes_pixels: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
