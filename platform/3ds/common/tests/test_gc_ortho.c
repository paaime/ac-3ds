#include "gc_ortho.h"

#include <math.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define NEAR_EQ(a, b) (fabsf((a) - (b)) < 1e-5f)

static void apply(const float m[4][4], float x, float y, float z, float out[4])
{
    for (int r = 0; r < 4; r++) {
        out[r] = m[r][0] * x + m[r][1] * y + m[r][2] * z + m[r][3];
    }
}

static void test_depth_maps_to_the_gx_range(void)
{
    /* GX clip space: eye z = -near -> -1, eye z = -far -> 0 (dolphin/mtx/mtx44.c). */
    float m[4][4];
    gc_ortho(m, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    float c[4];
    apply(m, 0.0f, 0.0f, 1.0f, c);
    CHECK(NEAR_EQ(c[2], -1.0f));
    apply(m, 0.0f, 0.0f, -1.0f, c);
    CHECK(NEAR_EQ(c[2], 0.0f));
    apply(m, 0.0f, 0.0f, 0.0f, c);
    CHECK(NEAR_EQ(c[2], -0.5f));
    CHECK(NEAR_EQ(c[3], 1.0f));
}

static void test_xy_map_the_box_to_minus_one_one(void)
{
    float m[4][4];
    gc_ortho(m, 0.0f, -480.0f, 0.0f, 640.0f, 0.0f, 2000.0f);
    float c[4];
    apply(m, 0.0f, 0.0f, 0.0f, c);
    CHECK(NEAR_EQ(c[0], -1.0f));
    CHECK(NEAR_EQ(c[1], 1.0f));
    apply(m, 640.0f, -480.0f, 0.0f, c);
    CHECK(NEAR_EQ(c[0], 1.0f));
    CHECK(NEAR_EQ(c[1], -1.0f));
}

static void test_last_row_is_affine(void)
{
    float m[4][4];
    gc_ortho(m, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    CHECK(m[3][0] == 0.0f && m[3][1] == 0.0f && m[3][2] == 0.0f && m[3][3] == 1.0f);
}

int main(void)
{
    test_depth_maps_to_the_gx_range();
    test_xy_map_the_box_to_minus_one_one();
    test_last_row_is_affine();
    if (failures == 0) {
        printf("gc_ortho: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
