/* Host tests for pica_tile.c. */
#include "pica_tile.h"

#include <stdio.h>
#include <stdlib.h>
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

/* Independent reference: tile-major order, then interleave the low three bits
 * of x and y one bit at a time (x first), as described on 3dbrew/GBATEK. */
static uint32_t reference_index(uint32_t x, uint32_t y, uint32_t width)
{
    uint32_t tiles_per_row = width / 8;
    uint32_t tile = (y / 8) * tiles_per_row + (x / 8);
    uint32_t in_tile = 0;
    for (uint32_t bit = 0; bit < 3; bit++) {
        in_tile |= ((x >> bit) & 1u) << (2 * bit);
        in_tile |= ((y >> bit) & 1u) << (2 * bit + 1);
    }
    return tile * 64 + in_tile;
}

/* Byte pattern unique per (texel, byte) so misplaced bytes are detectable. */
static uint8_t pattern_byte(uint32_t texel, uint32_t byte)
{
    return (uint8_t)(texel * 7u + byte * 61u + (texel >> 8) * 13u);
}

static uint8_t* make_linear_image(uint32_t width, uint32_t height, uint32_t bpp)
{
    uint8_t* image = malloc((size_t)width * height * bpp);
    for (uint32_t t = 0; t < width * height; t++) {
        for (uint32_t b = 0; b < bpp; b++) {
            image[t * bpp + b] = pattern_byte(t, b);
        }
    }
    return image;
}

static void test_known_indices(void)
{
    CHECK(pica_tiled_index(0, 0, 8) == 0);
    CHECK(pica_tiled_index(1, 0, 8) == 1);
    CHECK(pica_tiled_index(0, 1, 8) == 2);
    CHECK(pica_tiled_index(1, 1, 8) == 3);
    CHECK(pica_tiled_index(2, 0, 8) == 4);
    CHECK(pica_tiled_index(3, 1, 8) == 7);
    CHECK(pica_tiled_index(7, 7, 8) == 63);
    CHECK(pica_tiled_index(8, 0, 16) == 64);   /* second tile in the row */
    CHECK(pica_tiled_index(0, 8, 16) == 128);  /* first tile of the second row */
    CHECK(pica_tiled_index(9, 9, 16) == 195);  /* tile (1,1) = 3*64, texel (1,1) = 3 */
}

static void test_index_matches_reference(void)
{
    const uint32_t width = 32;
    const uint32_t height = 24;
    int mismatches = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            mismatches += pica_tiled_index(x, y, width) != reference_index(x, y, width);
        }
    }
    CHECK(mismatches == 0);
}

static void test_tile_places_every_texel(uint32_t bpp, bool flip_rows)
{
    const uint32_t width = 16;
    const uint32_t height = 24;
    uint8_t* linear = make_linear_image(width, height, bpp);
    uint8_t* tiled = calloc((size_t)width * height, bpp);
    int misplaced = 0;

    CHECK(pica_tile(tiled, linear, width, height, bpp, flip_rows));
    for (uint32_t y = 0; y < height; y++) {
        uint32_t stored_row = flip_rows ? height - 1 - y : y;
        for (uint32_t x = 0; x < width; x++) {
            uint32_t src = y * width + x;
            uint32_t dst = reference_index(x, stored_row, width);
            misplaced += memcmp(&tiled[dst * bpp], &linear[src * bpp], bpp) != 0;
        }
    }
    CHECK(misplaced == 0);
    free(linear);
    free(tiled);
}

static void test_flipped_first_row_is_stored_last(void)
{
    /* 8x16 image, 1 byte per texel: row 0 holds 0xA0.., the last row 0xF0.. */
    uint8_t linear[8 * 16];
    uint8_t tiled[8 * 16];
    for (uint32_t i = 0; i < sizeof(linear); i++) {
        linear[i] = (uint8_t)((i / 8 == 0) ? 0xA0 + i : (i / 8 == 15) ? 0xF0 + (i % 8) : 0);
    }
    CHECK(pica_tile(tiled, linear, 8, 16, 1, true));
    CHECK(tiled[pica_tiled_index(0, 15, 8)] == 0xA0); /* source (0,0) -> stored row 15 */
    CHECK(tiled[pica_tiled_index(7, 15, 8)] == 0xA7);
    CHECK(tiled[pica_tiled_index(0, 0, 8)] == 0xF0);  /* source last row -> stored row 0 */
}

static void test_rejects_invalid_arguments(void)
{
    uint8_t src[8 * 8 * 4] = { 0 };
    uint8_t dst[8 * 8 * 4];
    memset(dst, 0xAB, sizeof(dst));

    CHECK(!pica_tile(dst, src, 12, 8, 4, false));                       /* width not a multiple of 8 */
    CHECK(!pica_tile(dst, src, 8, 0, 4, false));                        /* zero height */
    CHECK(!pica_tile(dst, src, 8, 8, 0, false));                        /* zero bytes per texel */
    CHECK(!pica_tile(dst, src, 8, 8, 5, false));                        /* > 4 bytes per texel */
    CHECK(!pica_tile(dst, src, PICA_TEXTURE_MAX_DIM + 8, 8, 4, false)); /* too wide */
    CHECK(!pica_tile(NULL, src, 8, 8, 4, false));
    CHECK(!pica_tile(dst, NULL, 8, 8, 4, true));
    CHECK(!pica_tile(dst, dst, 8, 8, 4, true));                        /* in place */
    CHECK(!pica_tile(dst, src, 8, 12, 4, false));                       /* height not a multiple of 8 */
    CHECK(dst[0] == 0xAB && dst[sizeof(dst) - 1] == 0xAB);       /* untouched */
}

static void test_overlap_detection(void)
{
    enum { IMAGE_BYTES = 8 * 8 * 4 };
    static uint8_t buffer[2 * IMAGE_BYTES];

    CHECK(!pica_tile(buffer + 4, buffer, 8, 8, 4, false));            /* dst inside src */
    CHECK(!pica_tile(buffer, buffer + IMAGE_BYTES - 1, 8, 8, 4, false)); /* src inside dst */
    CHECK(pica_tile(buffer + IMAGE_BYTES, buffer, 8, 8, 4, false));   /* adjacent: allowed */
}

int main(void)
{
    test_known_indices();
    test_index_matches_reference();
    for (uint32_t bpp = 1; bpp <= 4; bpp++) {
        test_tile_places_every_texel(bpp, false);
        test_tile_places_every_texel(bpp, true);
    }
    test_flipped_first_row_is_stored_last();
    test_rejects_invalid_arguments();
    test_overlap_detection();

    if (failures != 0) {
        fprintf(stderr, "pica_tile: %d check(s) failed\n", failures);
        return 1;
    }
    printf("pica_tile: all tests passed\n");
    return 0;
}
