/* Host tests for gc_pattern.c. */
#include "gc_pattern.h"

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

/* Reference reader, written from the GX layout rather than from the module:
 * 16-bit texels in 4x4 blocks, blocks left to right then top to bottom,
 * texels row by row inside a block, big-endian. */
static uint16_t texel(const uint8_t* tex, uint32_t width, uint32_t x, uint32_t y)
{
    uint32_t block = (y / 4) * (width / 4) + x / 4;
    uint32_t inside = (y % 4) * 4 + x % 4;
    const uint8_t* p = tex + (block * 16 + inside) * 2;
    return (uint16_t)((p[0] << 8) | p[1]);
}

static void test_opaque_colour_packing(void)
{
    CHECK(gc_rgb5a3_opaque(255, 255, 255) == 0xFFFF);
    CHECK(gc_rgb5a3_opaque(0, 0, 0) == 0x8000);
    CHECK(gc_rgb5a3_opaque(255, 0, 0) == 0xFC00);
    CHECK(gc_rgb5a3_opaque(0, 255, 0) == 0x83E0);
    CHECK(gc_rgb5a3_opaque(0, 0, 255) == 0x801F);
    CHECK(gc_rgb5a3_opaque(8, 16, 24) == (0x8000 | (1 << 10) | (2 << 5) | 3));
}

static void test_size(void)
{
    CHECK(gc_rgb5a3_size(8, 8) == 128);
    CHECK(gc_rgb5a3_size(16, 4) == 128);
}

static void test_checker_cells(void)
{
    uint8_t tex[128];
    const uint16_t a = 0x8001, b = 0xFFFE;
    CHECK(gc_checker_rgb5a3(tex, sizeof(tex), 8, 8, 4, a, b));
    for (uint32_t y = 0; y < 8; y++) {
        for (uint32_t x = 0; x < 8; x++) {
            bool first = ((x / 4) + (y / 4)) % 2 == 0;
            CHECK(texel(tex, 8, x, y) == (first ? a : b));
        }
    }
}

static void test_checker_cells_smaller_than_a_block(void)
{
    uint8_t tex[128];
    CHECK(gc_checker_rgb5a3(tex, sizeof(tex), 8, 8, 1, 1, 2));
    CHECK(texel(tex, 8, 0, 0) == 1 && texel(tex, 8, 1, 0) == 2 && texel(tex, 8, 0, 1) == 2);
    CHECK(texel(tex, 8, 7, 7) == 1 && texel(tex, 8, 6, 7) == 2);
}

static void test_wide_texture(void)
{
    uint8_t tex[256];
    CHECK(gc_checker_rgb5a3(tex, sizeof(tex), 16, 8, 8, 7, 9));
    CHECK(texel(tex, 16, 7, 7) == 7 && texel(tex, 16, 8, 0) == 9 && texel(tex, 16, 15, 7) == 9);
}

/* Alpha of an RGB5A3 texel, 0..7 (7 for an opaque one). */
static int alpha_of(uint16_t v)
{
    return (v & 0x8000) ? 7 : (v >> 12) & 7;
}

static void test_disc(void)
{
    uint8_t tex[32 * 32 * 2];
    CHECK(gc_disc_rgb5a3(tex, sizeof(tex), 32, 255, 255, 255));
    CHECK(texel(tex, 32, 16, 16) == 0xFFFF);           /* the middle: opaque white */
    CHECK(texel(tex, 32, 2, 16) == 0xFFFF);            /* well inside, on the left */
    CHECK(alpha_of(texel(tex, 32, 0, 0)) == 0);        /* the corners are clear */
    CHECK(alpha_of(texel(tex, 32, 31, 31)) == 0);
    CHECK(alpha_of(texel(tex, 32, 0, 16)) < 7);        /* the edge is soft */
    for (uint32_t y = 0; y < 32; y++) {                /* symmetric */
        for (uint32_t x = 0; x < 32; x++) {
            CHECK(texel(tex, 32, x, y) == texel(tex, 32, 31 - x, y));
            CHECK(texel(tex, 32, x, y) == texel(tex, 32, x, 31 - y));
        }
    }
}

static void test_disc_colour_and_bad_arguments(void)
{
    uint8_t tex[8 * 8 * 2];
    CHECK(gc_disc_rgb5a3(tex, sizeof(tex), 8, 255, 0, 0));
    CHECK(texel(tex, 8, 4, 4) == 0xFC00);              /* opaque red */
    CHECK(!gc_disc_rgb5a3(tex, sizeof(tex), 6, 255, 0, 0));  /* not a block multiple */
    CHECK(!gc_disc_rgb5a3(tex, sizeof(tex) - 1, 8, 255, 0, 0));
    CHECK(!gc_disc_rgb5a3(NULL, sizeof(tex), 8, 255, 0, 0));
}

static int opaque_in_row(const uint8_t* tex, uint32_t dim, uint32_t y)
{
    int n = 0;
    for (uint32_t x = 0; x < dim; x++) {
        n += alpha_of(texel(tex, dim, x, y)) == 7 || (texel(tex, dim, x, y) & 0x8000) != 0;
    }
    return n;
}

static void test_arrow(void)
{
    uint8_t up[16 * 16 * 2];
    uint8_t down[16 * 16 * 2];
    CHECK(gc_arrow_rgb5a3(up, sizeof(up), 16, true, 255, 255, 255));
    CHECK(gc_arrow_rgb5a3(down, sizeof(down), 16, false, 255, 255, 255));
    CHECK(texel(up, 16, 8, 9) == 0xFFFF);                  /* inside: opaque white */
    CHECK(alpha_of(texel(up, 16, 0, 0)) == 0);             /* corners clear */
    CHECK(alpha_of(texel(up, 16, 15, 15)) == 0);
    CHECK(alpha_of(texel(up, 16, 8, 0)) == 0);             /* above the tip */
    CHECK(alpha_of(texel(up, 16, 8, 15)) == 0);            /* below the base */
    CHECK(opaque_in_row(up, 16, 6) < opaque_in_row(up, 16, 10)); /* up: wider towards the bottom */
    CHECK(opaque_in_row(up, 16, 10) > 0);
    for (uint32_t y = 0; y < 16; y++) {
        for (uint32_t x = 0; x < 16; x++) {
            CHECK(texel(up, 16, x, y) == texel(up, 16, 15 - x, y));     /* symmetric */
            CHECK(texel(up, 16, x, y) == texel(down, 16, x, 15 - y));   /* down: up, upside down */
        }
    }
}

static void test_arrow_colour_and_bad_arguments(void)
{
    uint8_t tex[16 * 16 * 2];
    CHECK(gc_arrow_rgb5a3(tex, sizeof(tex), 16, true, 0, 0, 255));
    CHECK(texel(tex, 16, 8, 9) == 0x801F);                 /* opaque blue */
    CHECK(!gc_arrow_rgb5a3(tex, sizeof(tex), 6, true, 0, 0, 0));
    CHECK(!gc_arrow_rgb5a3(tex, sizeof(tex) - 1, 16, true, 0, 0, 0));
    CHECK(!gc_arrow_rgb5a3(NULL, sizeof(tex), 16, true, 0, 0, 0));
}

static void test_rejects_bad_arguments(void)
{
    uint8_t tex[128];
    memset(tex, 0xAB, sizeof(tex));
    CHECK(!gc_checker_rgb5a3(tex, sizeof(tex), 6, 8, 2, 1, 2));  /* width not a block multiple */
    CHECK(!gc_checker_rgb5a3(tex, sizeof(tex), 8, 0, 2, 1, 2));  /* empty */
    CHECK(!gc_checker_rgb5a3(tex, sizeof(tex), 8, 8, 0, 1, 2));  /* no cell size */
    CHECK(!gc_checker_rgb5a3(tex, 127, 8, 8, 2, 1, 2));          /* buffer too small */
    CHECK(!gc_checker_rgb5a3(NULL, 128, 8, 8, 2, 1, 2));
    CHECK(tex[0] == 0xAB && tex[127] == 0xAB);                   /* untouched */
}

int main(void)
{
    test_opaque_colour_packing();
    test_size();
    test_checker_cells();
    test_checker_cells_smaller_than_a_block();
    test_wide_texture();
    test_rejects_bad_arguments();
    test_disc();
    test_disc_colour_and_bad_arguments();
    test_arrow();
    test_arrow_colour_and_bad_arguments();
    if (failures != 0) {
        fprintf(stderr, "gc_pattern: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gc_pattern: all tests passed\n");
    return 0;
}
