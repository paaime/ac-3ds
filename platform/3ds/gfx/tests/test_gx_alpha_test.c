/* Host tests for gx_alpha_test.c.
 *
 * Besides the hand-picked cases, every pair of GX compares over a grid of
 * references is checked by brute force: wherever the merged single test is
 * claimed exact, it must accept exactly the alphas both GX tests accept. */
#include "gx_alpha_test.h"

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

static bool compare(uint8_t func, int alpha, int ref)
{
    switch (func) {
    case GXA_NEVER: return false;
    case GXA_LESS: return alpha < ref;
    case GXA_EQUAL: return alpha == ref;
    case GXA_LEQUAL: return alpha <= ref;
    case GXA_GREATER: return alpha > ref;
    case GXA_NOTEQUAL: return alpha != ref;
    case GXA_GEQUAL: return alpha >= ref;
    default: return true;
    }
}

static bool passes(GxAlphaTest t, int alpha)
{
    return !t.enable || compare(t.func, alpha, t.ref);
}

static void test_emu64_cases(void)
{
    /* emu64 alpha_compare(): GEQUAL/ALWAYS for each half, always AND. */
    GxAlphaTest off = gx_alpha_test_merge(GXA_ALWAYS, 0, GXA_OP_AND, GXA_ALWAYS, 0);
    CHECK(!off.enable && off.exact);

    GxAlphaTest threshold = gx_alpha_test_merge(GXA_GEQUAL, 0x80, GXA_OP_AND, GXA_ALWAYS, 0x10);
    CHECK(threshold.enable && threshold.func == GXA_GEQUAL && threshold.ref == 0x80);

    GxAlphaTest edge = gx_alpha_test_merge(GXA_ALWAYS, 0x80, GXA_OP_AND, GXA_GEQUAL, 0x10);
    CHECK(edge.enable && edge.func == GXA_GEQUAL && edge.ref == 0x10);

    GxAlphaTest both = gx_alpha_test_merge(GXA_GEQUAL, 0x20, GXA_OP_AND, GXA_GEQUAL, 0x60);
    CHECK(both.enable && both.func == GXA_GEQUAL && both.ref == 0x60 && both.exact);
}

static void test_bounds_merge_to_the_stricter(void)
{
    GxAlphaTest gt = gx_alpha_test_merge(GXA_GREATER, 0x40, GXA_OP_AND, GXA_GEQUAL, 0x41);
    CHECK(gt.exact && passes(gt, 0x41) && !passes(gt, 0x40));

    GxAlphaTest above_max = gx_alpha_test_merge(GXA_GREATER, 0xFF, GXA_OP_AND, GXA_ALWAYS, 0);
    CHECK(above_max.exact && !passes(above_max, 0xFF));

    GxAlphaTest le = gx_alpha_test_merge(GXA_LESS, 0x40, GXA_OP_AND, GXA_LEQUAL, 0x20);
    CHECK(le.exact && passes(le, 0x20) && !passes(le, 0x21));

    GxAlphaTest below_min = gx_alpha_test_merge(GXA_LESS, 0, GXA_OP_AND, GXA_ALWAYS, 0);
    CHECK(below_min.exact && !passes(below_min, 0));
}

static void test_approximations_are_flagged(void)
{
    /* A range (lower AND upper bound) has no single PICA test: keep the lower one. */
    GxAlphaTest range = gx_alpha_test_merge(GXA_GEQUAL, 0x20, GXA_OP_AND, GXA_LEQUAL, 0xC0);
    CHECK(!range.exact && range.enable && range.func == GXA_GEQUAL && range.ref == 0x20);

    GxAlphaTest range2 = gx_alpha_test_merge(GXA_LEQUAL, 0xC0, GXA_OP_AND, GXA_GREATER, 0x20);
    CHECK(!range2.exact && range2.func == GXA_GREATER && range2.ref == 0x20);

    GxAlphaTest other = gx_alpha_test_merge(GXA_EQUAL, 0x20, GXA_OP_AND, GXA_NOTEQUAL, 0x30);
    CHECK(!other.exact && other.func == GXA_EQUAL);

    GxAlphaTest or_op = gx_alpha_test_merge(GXA_GEQUAL, 0x20, GXA_OP_OR, GXA_GEQUAL, 0x30);
    CHECK(!or_op.exact && !or_op.enable);
}

static void test_never_and_unknown(void)
{
    GxAlphaTest never = gx_alpha_test_merge(GXA_GEQUAL, 0x20, GXA_OP_AND, GXA_NEVER, 0);
    CHECK(never.exact && never.enable && never.func == GXA_NEVER);

    GxAlphaTest unknown = gx_alpha_test_merge(9, 0x20, GXA_OP_AND, GXA_ALWAYS, 0);
    CHECK(!unknown.exact && !unknown.enable);
}

static void test_exhaustive_exactness(void)
{
    static const int REFS[] = { 0, 1, 0x40, 0x80, 0xFE, 0xFF };
    const int nrefs = (int)(sizeof(REFS) / sizeof(REFS[0]));
    int exact_pairs = 0;
    for (uint8_t c0 = 0; c0 < 8; c0++) {
        for (uint8_t c1 = 0; c1 < 8; c1++) {
            for (int i = 0; i < nrefs; i++) {
                for (int j = 0; j < nrefs; j++) {
                    GxAlphaTest t = gx_alpha_test_merge(c0, (uint8_t)REFS[i], GXA_OP_AND, c1, (uint8_t)REFS[j]);
                    if (!t.exact) {
                        continue;
                    }
                    exact_pairs++;
                    for (int a = 0; a < 256; a++) {
                        bool want = compare(c0, a, REFS[i]) && compare(c1, a, REFS[j]);
                        if (passes(t, a) != want) {
                            fprintf(stderr, "c0 %d r0 %d c1 %d r1 %d alpha %d\n", c0, REFS[i], c1, REFS[j], a);
                            failures++;
                            return;
                        }
                    }
                }
            }
        }
    }
    CHECK(exact_pairs > 0);
}

int main(void)
{
    test_emu64_cases();
    test_bounds_merge_to_the_stricter();
    test_approximations_are_flagged();
    test_never_and_unknown();
    test_exhaustive_exactness();
    if (failures != 0) {
        fprintf(stderr, "gx_alpha_test: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gx_alpha_test: all tests passed\n");
    return 0;
}
