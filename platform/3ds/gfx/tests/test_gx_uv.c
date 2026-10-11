/* Host tests for gx_uv.c. */
#include "gx_uv.h"

#include <math.h>
#include <stdbool.h>
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

/* What the vertex shader does: (s, t) = rows . (s_in, t_in, 0, 1). */
static void shade(const float r0[4], const float r1[4], float s, float t, float* out_s, float* out_t)
{
    *out_s = r0[0] * s + r0[1] * t + r0[3];
    *out_t = r1[0] * s + r1[1] * t + r1[3];
}

static const float IDENTITY0[4] = { 1, 0, 0, 0 };
static const float IDENTITY1[4] = { 0, 1, 0, 0 };

static void test_scale_multiplies_the_rows(void)
{
    float o0[4], o1[4], s, t;
    gx_uv_apply(gx_uv_scale(0.5f, 0.25f), IDENTITY0, IDENTITY1, o0, o1);
    shade(o0, o1, 1.0f, 1.0f, &s, &t);
    CHECK(NEAR_EQ(s, 0.5f) && NEAR_EQ(t, 0.25f));
}

static void test_apply_after_a_texture_matrix(void)
{
    /* texture matrix: s' = 2s + 0.1, t' = t + 0.5 */
    const float r0[4] = { 2, 0, 0, 0.1f };
    const float r1[4] = { 0, 1, 0, 0.5f };
    GxUvAffine swap = { { { 0, 1, 0 }, { 1, 0, 0 } } }; /* (s, t) -> (t, s) */
    float o0[4], o1[4], s, t;
    gx_uv_apply(swap, r0, r1, o0, o1);
    shade(o0, o1, 1.0f, 2.0f, &s, &t);
    CHECK(NEAR_EQ(s, 2.5f) && NEAR_EQ(t, 2.1f));
}

static void test_translation_lands_on_the_constant_term(void)
{
    GxUvAffine shift = { { { 1, 0, 0.25f }, { 0, 1, -0.5f } } };
    float o0[4], o1[4], s, t;
    gx_uv_apply(shift, IDENTITY0, IDENTITY1, o0, o1);
    shade(o0, o1, 0.0f, 0.0f, &s, &t);
    CHECK(NEAR_EQ(s, 0.25f) && NEAR_EQ(t, -0.5f));
}

/* The copied framebuffer is 240 x 400 (portrait) inside a 256 x 512 texture. */
static GxUvAffine copy_mapping(void)
{
    GxUvCopyLayout l = { 240, 400, 256, 512, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    return gx_uv_rotated_copy(l);
}

static void map(GxUvAffine a, float s, float t, float* out_s, float* out_t)
{
    *out_s = a.m[0][0] * s + a.m[0][1] * t + a.m[0][2];
    *out_t = a.m[1][0] * s + a.m[1][1] * t + a.m[1][2];
}

static void test_rotated_copy_stays_inside_the_copied_area(void)
{
    GxUvAffine a = copy_mapping();
    const float corners[4][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
    for (int i = 0; i < 4; i++) {
        float s, t;
        map(a, corners[i][0], corners[i][1], &s, &t);
        CHECK(s >= -1e-5f && s <= 240.0f / 256.0f + 1e-5f);
        CHECK(t >= 1.0f - 400.0f / 512.0f - 1e-5f && t <= 1.0f + 1e-5f);
    }
}

static void test_rotated_copy_turns_screen_axes_into_framebuffer_axes(void)
{
    /* Moving right on screen (s) walks the framebuffer's 400-pixel axis
     * (texture t); moving down (t) walks its 240-pixel axis (texture s). */
    GxUvAffine a = copy_mapping();
    CHECK(NEAR_EQ(a.m[0][0], 0.0f) && NEAR_EQ(fabsf(a.m[0][1]), 240.0f / 256.0f));
    CHECK(NEAR_EQ(a.m[1][1], 0.0f) && NEAR_EQ(fabsf(a.m[1][0]), 400.0f / 512.0f));
}

static void test_rotated_copy_orientation(void)
{
    /* Checked in Azahar: the screen's left edge is framebuffer row 0 (texture
     * t = 1) and its top edge the framebuffer's last column (texture s =
     * 240/256); the bottom-right corner is row 399, column 0. */
    GxUvAffine a = copy_mapping();
    float s, t;
    map(a, 0.0f, 0.0f, &s, &t);
    CHECK(NEAR_EQ(s, 240.0f / 256.0f) && NEAR_EQ(t, 1.0f));
    map(a, 1.0f, 1.0f, &s, &t);
    CHECK(NEAR_EQ(s, 0.0f) && NEAR_EQ(t, 1.0f - 400.0f / 512.0f));
}

static void test_source_rectangle_selects_part_of_the_screen(void)
{
    /* A copy of the EFB's right half: its s = 0 is the screen's middle. */
    GxUvCopyLayout full = { 240, 400, 256, 512, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    GxUvCopyLayout right = { 240, 400, 256, 512, 0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    GxUvAffine a = gx_uv_rotated_copy(full);
    GxUvAffine b = gx_uv_rotated_copy(right);
    float s1, t1, s2, t2;
    map(a, 0.5f, 0.3f, &s1, &t1);
    map(b, 0.0f, 0.3f, &s2, &t2);
    CHECK(NEAR_EQ(s1, s2) && NEAR_EQ(t1, t2));
    map(a, 1.0f, 0.3f, &s1, &t1);
    map(b, 1.0f, 0.3f, &s2, &t2);
    CHECK(NEAR_EQ(s1, s2) && NEAR_EQ(t1, t2));
}

static bool same_affine(GxUvAffine a, GxUvAffine b)
{
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 3; c++) {
            if (!NEAR_EQ(a.m[r][c], b.m[r][c])) {
                return false;
            }
        }
    }
    return true;
}

/* With the 4:3 picture in the screen's middle (x 0.1..0.9 of it), the copy's
 * EFB area lands there in the framebuffer: as a whole-screen copy of that part. */
static void test_image_area_places_the_copy(void)
{
    GxUvCopyLayout pillar = { 240, 400, 256, 512, 0.0f, 0.0f, 1.0f, 1.0f, 0.1f, 0.0f, 0.8f, 1.0f };
    GxUvCopyLayout same = { 240, 400, 256, 512, 0.1f, 0.0f, 0.8f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    CHECK(same_affine(gx_uv_rotated_copy(pillar), gx_uv_rotated_copy(same)));
    GxUvCopyLayout half = { 240, 400, 256, 512, 0.5f, 0.0f, 0.5f, 1.0f, 0.1f, 0.0f, 0.8f, 1.0f };
    GxUvCopyLayout half_same = { 240, 400, 256, 512, 0.5f, 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    CHECK(same_affine(gx_uv_rotated_copy(half), gx_uv_rotated_copy(half_same)));
}

int main(void)
{
    test_scale_multiplies_the_rows();
    test_apply_after_a_texture_matrix();
    test_translation_lands_on_the_constant_term();
    test_rotated_copy_stays_inside_the_copied_area();
    test_rotated_copy_turns_screen_axes_into_framebuffer_axes();
    test_rotated_copy_orientation();
    test_source_rectangle_selects_part_of_the_screen();
    test_image_area_places_the_copy();
    if (failures == 0) {
        printf("gx_uv: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
