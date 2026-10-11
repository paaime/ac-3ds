/* Host tests for gx_fog.c. */
#include "gx_fog.h"

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

#define NEAR_EQ(a, b, eps) (fabsf((a) - (b)) < (eps))

/* The PICA depth (1 = near plane, 0 = far) at eye distance d. */
static float depth_of_distance(float d, float n, float f)
{
    return (f * n / d - n) / (f - n); /* inverse of citro3d's FogLut_CalcZ */
}

static void test_none_is_off(void)
{
    CHECK(!gx_fog_enabled(GX_FOG_TYPE_NONE));
    CHECK(gx_fog_enabled(GX_FOG_TYPE_PERSP_LIN));
    CHECK(gx_fog_enabled(GX_FOG_TYPE_ORTHO_LIN));
}

static void test_linear_perspective_visibility(void)
{
    GxFogParams p = { GX_FOG_TYPE_PERSP_LIN, 100.0f, 400.0f, 10.0f, 1000.0f };
    float data[GX_FOG_LUT_FLOATS];
    gx_fog_lut(p, data);
    /* The sample nearest the near plane is clear, the one at the far plane fully fogged. */
    CHECK(NEAR_EQ(data[GX_FOG_LUT_SIZE - 1], 1.0f, 1e-4f));
    CHECK(NEAR_EQ(data[0], 0.0f, 1e-4f));
    /* Midway between start and end (distance 250): half fogged. */
    float vis = gx_fog_visibility(p, depth_of_distance(250.0f, 10.0f, 1000.0f));
    CHECK(NEAR_EQ(vis, 0.5f, 1e-4f));
}

static void test_visibility_never_leaves_0_1(void)
{
    GxFogParams p = { GX_FOG_TYPE_PERSP_LIN, 300.0f, 200.0f, 10.0f, 1000.0f }; /* end before start */
    for (int i = 0; i <= GX_FOG_LUT_SIZE; i++) {
        float v = gx_fog_visibility(p, (float)i / GX_FOG_LUT_SIZE);
        CHECK(v >= 0.0f && v <= 1.0f);
    }
}

static void test_deltas_link_neighbouring_samples(void)
{
    GxFogParams p = { GX_FOG_TYPE_PERSP_LIN, 50.0f, 900.0f, 10.0f, 1000.0f };
    float data[GX_FOG_LUT_FLOATS];
    gx_fog_lut(p, data);
    for (int i = 0; i < GX_FOG_LUT_SIZE - 1; i++) {
        CHECK(NEAR_EQ(data[i] + data[GX_FOG_LUT_SIZE + i], data[i + 1], 1e-5f));
    }
    float last = gx_fog_visibility(p, 1.0f);
    CHECK(NEAR_EQ(data[GX_FOG_LUT_SIZE - 1] + data[2 * GX_FOG_LUT_SIZE - 1], last, 1e-5f));
}

static void test_orthographic_is_linear_in_depth(void)
{
    GxFogParams p = { GX_FOG_TYPE_ORTHO_LIN, 0.0f, 100.0f, 0.0f, 100.0f };
    CHECK(NEAR_EQ(gx_fog_visibility(p, 1.0f), 1.0f, 1e-5f)); /* near plane */
    CHECK(NEAR_EQ(gx_fog_visibility(p, 0.0f), 0.0f, 1e-5f)); /* far plane */
    CHECK(NEAR_EQ(gx_fog_visibility(p, 0.25f), 0.25f, 1e-5f));
}

static void test_exponential_curves(void)
{
    /* GX: EXP f = 1 - 2^(-8z), EXP2 f = 1 - 2^(-8z^2), with z the linear ramp. */
    GxFogParams e = { GX_FOG_TYPE_ORTHO_LIN + 2, 0.0f, 100.0f, 0.0f, 100.0f };  /* ORTHO_EXP */
    GxFogParams e2 = { GX_FOG_TYPE_ORTHO_LIN + 3, 0.0f, 100.0f, 0.0f, 100.0f }; /* ORTHO_EXP2 */
    float z = 0.5f; /* depth 0.5 is halfway */
    CHECK(NEAR_EQ(gx_fog_visibility(e, 0.5f), powf(2.0f, -8.0f * z), 1e-5f));
    CHECK(NEAR_EQ(gx_fog_visibility(e2, 0.5f), powf(2.0f, -8.0f * z * z), 1e-5f));
}

static void test_reverse_exponential_curves(void)
{
    /* GX: REVEXP f = 2^(-8(1-z)), REVEXP2 f = 2^(-8(1-z)^2). */
    GxFogParams r = { GX_FOG_TYPE_ORTHO_LIN + 4, 0.0f, 100.0f, 0.0f, 100.0f };  /* ORTHO_REVEXP */
    GxFogParams r2 = { GX_FOG_TYPE_ORTHO_LIN + 5, 0.0f, 100.0f, 0.0f, 100.0f }; /* ORTHO_REVEXP2 */
    float z = 0.75f; /* depth 0.25 */
    CHECK(NEAR_EQ(gx_fog_visibility(r, 0.25f), 1.0f - powf(2.0f, -8.0f * (1.0f - z)), 1e-5f));
    CHECK(NEAR_EQ(gx_fog_visibility(r2, 0.25f), 1.0f - powf(2.0f, -8.0f * (1.0f - z) * (1.0f - z)), 1e-5f));
}

static void test_disabled_fog_is_clear(void)
{
    GxFogParams off = { GX_FOG_TYPE_NONE, 0.0f, 1.0f, 1.0f, 100.0f };
    CHECK(gx_fog_visibility(off, 0.0f) == 1.0f);
    CHECK(!gx_fog_enabled(1)); /* curve 1 is not a GX fog function */
}

static void test_zero_length_ramp_is_a_step(void)
{
    GxFogParams step = { GX_FOG_TYPE_ORTHO_LIN, 50.0f, 50.0f, 0.0f, 100.0f };
    CHECK(gx_fog_visibility(step, 0.9f) == 1.0f); /* distance 10: before the step */
    CHECK(gx_fog_visibility(step, 0.1f) == 0.0f); /* distance 90: past it */
}

static void test_degenerate_planes_put_everything_far(void)
{
    GxFogParams bad = { GX_FOG_TYPE_PERSP_LIN, 10.0f, 20.0f, 0.0f, 0.0f }; /* near = far = 0 */
    CHECK(gx_fog_visibility(bad, 0.0f) == 1.0f); /* distance far (0) is before the ramp */
}

int main(void)
{
    test_none_is_off();
    test_linear_perspective_visibility();
    test_visibility_never_leaves_0_1();
    test_deltas_link_neighbouring_samples();
    test_orthographic_is_linear_in_depth();
    test_exponential_curves();
    test_reverse_exponential_curves();
    test_disabled_fog_is_clear();
    test_zero_length_ramp_is_a_step();
    test_degenerate_planes_put_everything_far();
    if (failures == 0) {
        printf("gx_fog: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
