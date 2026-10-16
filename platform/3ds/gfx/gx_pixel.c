#include "gx_pixel.h"

#include <3ds.h>
#include <citro3d.h>

#include "gx_alpha_test.h"

/* Last values sent: citro3d's setters re-send the whole effect block even when nothing changed. */
static struct {
    bool valid;
    GPU_TESTFUNC depth_func;
    GPU_WRITEMASK depth_mask;
    GPU_BLENDEQUATION blend_eq;
    GPU_BLENDFACTOR blend_src, blend_dst;
    bool alpha_enable;
    GPU_TESTFUNC alpha_func;
    uint8_t alpha_ref;
    GPU_CULLMODE cull;
} P;

void gxp_invalidate(void)
{
    P.valid = false;
}

static GPU_TESTFUNC compare_direct(uint8_t gx)
{
    static const GPU_TESTFUNC MAP[8] = { GPU_NEVER, GPU_LESS, GPU_EQUAL, GPU_LEQUAL,
                                         GPU_GREATER, GPU_NOTEQUAL, GPU_GEQUAL, GPU_ALWAYS };
    return MAP[gx & 7];
}

/* citro3d's default depth map stores -z (nearer is larger): the ordering compares are mirrored. */
static GPU_TESTFUNC compare_depth(uint8_t gx)
{
    static const GPU_TESTFUNC MAP[8] = { GPU_NEVER, GPU_GREATER, GPU_EQUAL, GPU_GEQUAL,
                                         GPU_LESS, GPU_NOTEQUAL, GPU_LEQUAL, GPU_ALWAYS };
    return MAP[gx & 7];
}

void gxp_apply_depth(const GxbState* st)
{
    GPU_WRITEMASK mask = 0;
    if (st->color_update) {
        mask |= GPU_WRITE_RED | GPU_WRITE_GREEN | GPU_WRITE_BLUE;
    }
    if (st->alpha_update) {
        mask |= GPU_WRITE_ALPHA;
    }
    if (st->z_enable && st->z_update) { /* GX writes no depth with the test off */
        mask |= GPU_WRITE_DEPTH;
    }
    GPU_TESTFUNC func = st->z_enable ? compare_depth(st->z_func) : GPU_ALWAYS;
    if (P.valid && func == P.depth_func && mask == P.depth_mask) {
        return;
    }
    C3D_DepthTest(true, func, mask);
    P.depth_func = func;
    P.depth_mask = mask;
}

static GPU_BLENDFACTOR blend_factor(uint8_t gx, bool is_src)
{
    switch (gx) {
    case 0: return GPU_ZERO;
    case 1: return GPU_ONE;
    case 2: return is_src ? GPU_DST_COLOR : GPU_SRC_COLOR; /* GX_BL_DSTCLR / GX_BL_SRCCLR */
    case 3: return is_src ? GPU_ONE_MINUS_DST_COLOR : GPU_ONE_MINUS_SRC_COLOR;
    case 4: return GPU_SRC_ALPHA;
    case 5: return GPU_ONE_MINUS_SRC_ALPHA;
    case 6: return GPU_DST_ALPHA;
    default: return GPU_ONE_MINUS_DST_ALPHA;
    }
}

void gxp_apply_blend(const GxbState* st)
{
    GPU_BLENDEQUATION eq = GPU_BLEND_ADD;
    GPU_BLENDFACTOR src = GPU_ONE;
    GPU_BLENDFACTOR dst = GPU_ZERO;
    if (st->blend_type == GXB_BM_BLEND) {
        src = blend_factor(st->blend_src, true);
        dst = blend_factor(st->blend_dst, false);
    } else if (st->blend_type == GXB_BM_SUBTRACT) {
        eq = GPU_BLEND_REVERSE_SUBTRACT;
        dst = GPU_ONE;
    }
    if (P.valid && eq == P.blend_eq && src == P.blend_src && dst == P.blend_dst) {
        return;
    }
    C3D_AlphaBlend(eq, eq, src, dst, src, dst);
    P.blend_eq = eq;
    P.blend_src = src;
    P.blend_dst = dst;
}

bool gxp_apply_alpha_test(const GxbState* st)
{
    GxAlphaTest t = gx_alpha_test_merge(st->alpha_comp0, st->alpha_ref0, st->alpha_op, st->alpha_comp1,
                                        st->alpha_ref1);
    GPU_TESTFUNC func = t.enable ? compare_direct(t.func) : GPU_ALWAYS;
    if (!P.valid || t.enable != P.alpha_enable || func != P.alpha_func || t.ref != P.alpha_ref) {
        C3D_AlphaTest(t.enable, func, t.ref);
        P.alpha_enable = t.enable;
        P.alpha_func = func;
        P.alpha_ref = t.ref;
    }
    return t.exact;
}

bool gxp_apply_cull(const GxbState* st)
{
    GPU_CULLMODE mode;
    switch (st->cull_mode) {
    case GXB_CULL_NONE: mode = GPU_CULL_NONE; break;
    case GXB_CULL_FRONT: mode = GPU_CULL_FRONT_CCW; break;
    case GXB_CULL_ALL: return false;
    default: mode = GPU_CULL_BACK_CCW; break;
    }
    if (!P.valid || mode != P.cull) {
        C3D_CullFace(mode);
        P.cull = mode;
    }
    return true;
}

void gxp_mark_applied(void)
{
    P.valid = true;
}
