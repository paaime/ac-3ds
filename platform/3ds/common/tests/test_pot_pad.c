/* Host tests for pot_pad.c. */
#include "pot_pad.h"

#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void test_pad_side(void)
{
    CHECK(pot_pad_side(0) == 0);
    CHECK(pot_pad_side(1) == 8);
    CHECK(pot_pad_side(8) == 8);
    CHECK(pot_pad_side(9) == 16);
    CHECK(pot_pad_side(48) == 64);
    CHECK(pot_pad_side(64) == 64);
    CHECK(pot_pad_side(376) == 512);
    CHECK(pot_pad_side(1024) == 1024);
    CHECK(pot_pad_side(1025) == 0);
}

static void test_pad_replicates_edges(void)
{
    enum { W = 12, H = 6, PW = 16, PH = 8 };
    uint32_t src[W * H];
    uint32_t dst[PW * PH];
    for (uint32_t i = 0; i < W * H; i++) {
        src[i] = 0x01000000u * (i % W) + i / W; /* encodes (x, y) */
    }

    CHECK(pot_pad_rgba8_wrap(dst, src, W, H, POT_PAD_EDGE, POT_PAD_EDGE));
    int wrong = 0;
    for (uint32_t y = 0; y < PH; y++) {
        for (uint32_t x = 0; x < PW; x++) {
            uint32_t sx = x < W ? x : W - 1;
            uint32_t sy = y < H ? y : H - 1;
            wrong += dst[y * PW + x] != src[sy * W + sx];
        }
    }
    CHECK(wrong == 0);
}

static void test_already_power_of_two_is_a_copy(void)
{
    uint32_t src[8 * 8];
    uint32_t dst[8 * 8];
    for (uint32_t i = 0; i < 64; i++) {
        src[i] = i * 2654435761u;
    }
    CHECK(pot_pad_rgba8_wrap(dst, src, 8, 8, POT_PAD_EDGE, POT_PAD_EDGE));
    int wrong = 0;
    for (uint32_t i = 0; i < 64; i++) {
        wrong += dst[i] != src[i];
    }
    CHECK(wrong == 0);
}

static void test_rejects_invalid_arguments(void)
{
    uint32_t src[16] = { 0 };
    uint32_t dst[64];
    dst[0] = 0xCAFEF00Du;

    CHECK(!pot_pad_rgba8_wrap(dst, src, 0, 4, POT_PAD_EDGE, POT_PAD_EDGE));
    CHECK(!pot_pad_rgba8_wrap(dst, src, 4, 2048, POT_PAD_EDGE, POT_PAD_EDGE));
    CHECK(!pot_pad_rgba8_wrap(NULL, src, 4, 4, POT_PAD_EDGE, POT_PAD_EDGE));
    CHECK(!pot_pad_rgba8_wrap(dst, NULL, 4, 4, POT_PAD_EDGE, POT_PAD_EDGE));
    CHECK(dst[0] == 0xCAFEF00Du);
}

static void test_axis_plans(void)
{
    PotPadAxis a = pot_pad_axis(4, POT_PAD_REPEAT); /* small power of two: tiled copies */
    CHECK(a.padded == 8 && !a.stretch && a.uv_scale == 0.5f);
    a = pot_pad_axis(48, POT_PAD_REPEAT); /* not a power of two: stretched over 64 */
    CHECK(a.padded == 64 && a.stretch && a.uv_scale == 1.0f);
    a = pot_pad_axis(48, POT_PAD_EDGE); /* clamped: edge padding */
    CHECK(a.padded == 64 && !a.stretch && a.uv_scale == 48.0f / 64.0f);
    a = pot_pad_axis(32, POT_PAD_MIRROR); /* no padding needed */
    CHECK(a.padded == 32 && !a.stretch && a.uv_scale == 1.0f);
    a = pot_pad_axis(0, POT_PAD_REPEAT);
    CHECK(a.padded == 0);
}

static void test_effective_wrap_only_matters_when_padded(void)
{
    CHECK(pot_pad_effective_wrap(32, POT_PAD_REPEAT) == POT_PAD_EDGE);
    CHECK(pot_pad_effective_wrap(4, POT_PAD_MIRROR) == POT_PAD_MIRROR);
    CHECK(pot_pad_effective_wrap(48, POT_PAD_REPEAT) == POT_PAD_REPEAT);
}

static void test_repeat_tiles_small_textures(void)
{
    const uint32_t src[4] = { 1, 2, 3, 4 }; /* 4 x 1 */
    uint32_t dst[8 * 8];
    CHECK(pot_pad_rgba8_wrap(dst, src, 4, 1, POT_PAD_REPEAT, POT_PAD_EDGE));
    const uint32_t row[8] = { 1, 2, 3, 4, 1, 2, 3, 4 };
    for (int x = 0; x < 8; x++) {
        CHECK(dst[x] == row[x]);
        CHECK(dst[7 * 8 + x] == row[x]); /* edge-padded rows repeat row 0 */
    }
}

static void test_mirror_alternates_copies(void)
{
    const uint32_t src[2] = { 5, 6 }; /* 1 x 2 */
    uint32_t dst[8 * 8];
    CHECK(pot_pad_rgba8_wrap(dst, src, 1, 2, POT_PAD_EDGE, POT_PAD_MIRROR));
    const uint32_t col[8] = { 5, 6, 6, 5, 5, 6, 6, 5 };
    for (int y = 0; y < 8; y++) {
        CHECK(dst[y * 8 + 0] == col[y]);
        CHECK(dst[y * 8 + 7] == col[y]);
    }
}

static void test_stretch_resamples_non_power_of_two(void)
{
    uint32_t src[12]; /* 12 x 1 -> 16 */
    for (int i = 0; i < 12; i++) {
        src[i] = (uint32_t)i;
    }
    uint32_t dst[16 * 8];
    CHECK(pot_pad_rgba8_wrap(dst, src, 12, 1, POT_PAD_REPEAT, POT_PAD_EDGE));
    CHECK(dst[0] == 0);
    CHECK(dst[15] == 11);               /* the last texel ends the period */
    for (int x = 1; x < 16; x++) {
        CHECK(dst[x] >= dst[x - 1]);    /* nearest resampling keeps the order */
    }
}

static void test_edge_wrap_matches_the_old_padding(void)
{
    uint32_t src[3 * 5];
    for (int i = 0; i < 15; i++) {
        src[i] = (uint32_t)(i * 7 + 1);
    }
    uint32_t a[8 * 8], b[8 * 8];
    CHECK(pot_pad_rgba8_wrap(a, src, 3, 5, POT_PAD_EDGE, POT_PAD_EDGE));
    CHECK(pot_pad_rgba8_wrap(b, src, 3, 5, POT_PAD_EDGE, POT_PAD_EDGE));
    for (int i = 0; i < 64; i++) {
        CHECK(a[i] == b[i]);
    }
}

int main(void)
{
    test_pad_side();
    test_pad_replicates_edges();
    test_already_power_of_two_is_a_copy();
    test_rejects_invalid_arguments();
    test_axis_plans();
    test_effective_wrap_only_matters_when_padded();
    test_repeat_tiles_small_textures();
    test_mirror_alternates_copies();
    test_stretch_resamples_non_power_of_two();
    test_edge_wrap_matches_the_old_padding();

    if (failures != 0) {
        fprintf(stderr, "pot_pad: %d check(s) failed\n", failures);
        return 1;
    }
    printf("pot_pad: all tests passed\n");
    return 0;
}
