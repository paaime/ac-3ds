/* gx_bind.c - a draw's textures on the PICA's units (see gx_bind.h). */
#include "gx_bind.h"

#include <string.h>

#include "gx_efb.h"
#include "gx_texcache.h"

#define GX_TF_C14X2 0x0A

typedef struct {
    const C3D_Tex* tex;
    uint32_t params; /* wrap and filter */
    uint32_t serial; /* texture uploads + EFB copies when it was bound */
    bool valid;
} UnitBinding;

static struct {
    UnitBinding units[PICA_TEXTURE_UNITS];
    C3D_Tex* fallback;
    bool sharp;
} B;

void gxbind_set_fallback(C3D_Tex* fallback)
{
    B.fallback = fallback;
}

void gxbind_set_sharp(bool on)
{
    B.sharp = on;
}

void gxbind_invalidate(void)
{
    memset(B.units, 0, sizeof(B.units));
}

static GPU_TEXTURE_WRAP_PARAM wrap_mode(uint8_t gx)
{
    switch (gx) {
    case 1: return GPU_REPEAT;          /* GX_REPEAT */
    case 2: return GPU_MIRRORED_REPEAT; /* GX_MIRROR */
    default: return GPU_CLAMP_TO_EDGE;  /* GX_CLAMP */
    }
}

static GPU_TEXTURE_FILTER_PARAM filter_mode(uint8_t gx)
{
    return gx == 0 ? GPU_NEAREST : GPU_LINEAR; /* GX_NEAR */
}

/* The serial changes whenever texels may have changed (a reused cache slot, a
 * new EFB copy): the bind is then redone, which also clears the PICA's texture cache. */
void gxbind_texture(int unit, C3D_Tex* tex, GPU_TEXTURE_WRAP_PARAM wrap_s, GPU_TEXTURE_WRAP_PARAM wrap_t,
                    GPU_TEXTURE_FILTER_PARAM mag, GPU_TEXTURE_FILTER_PARAM min)
{
    if (unit < 0 || unit >= PICA_TEXTURE_UNITS || tex == NULL) {
        return;
    }
    uint32_t params = (uint32_t)wrap_s | ((uint32_t)wrap_t << 8) | ((uint32_t)mag << 16) | ((uint32_t)min << 24);
    uint32_t serial = gxtc_upload_serial() + gxe_copy_count();
    UnitBinding* b = &B.units[unit];
    if (b->valid && b->tex == tex && b->params == params && b->serial == serial) {
        return;
    }
    C3D_TexSetWrap(tex, wrap_s, wrap_t);
    C3D_TexSetFilter(tex, mag, min);
    C3D_TexBind(unit, tex);
    *b = (UnitBinding){ tex, params, serial, true };
    /* Wrap and filter belong to the C3D_Tex, not the unit: another unit bound
     * to the same texture now has these settings too, so its entry is stale. */
    for (int u = 0; u < PICA_TEXTURE_UNITS; u++) {
        if (u != unit && B.units[u].valid && B.units[u].tex == tex && B.units[u].params != params) {
            B.units[u].valid = false;
        }
    }
}

/* Every unit needs a texture, even one no draw samples: once the vertex shader
 * outputs texcoords, the real GPU hangs on a unit never given one (Azahar
 * doesn't). */
static void bind_unused(int unit)
{
    gxbind_texture(unit, B.fallback, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE, GPU_NEAREST, GPU_NEAREST);
}

void gxbind_blit(C3D_Tex* tex)
{
    gxbind_texture(0, tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE, GPU_NEAREST, GPU_NEAREST);
    for (int unit = 1; unit < PICA_TEXTURE_UNITS; unit++) {
        bind_unused(unit); /* in 4:3 the bars are a frame's first draw */
    }
}

/* A texture that can't be decoded (C14X2, bad size, cache full) samples the
 * white fallback, so the draw keeps its vertex and constant colours. */
void gxbind_draw(const GxbState* st, const PicaCombiner* p, GxUvAffine uv[GXBIND_TEXGEN_UNITS], GxBindCounts* counts)
{
    for (int unit = GXBIND_TEXGEN_UNITS; unit < PICA_TEXTURE_UNITS; unit++) {
        bind_unused(unit); /* the shader has no texcoord for these */
    }
    for (int unit = 0; unit < GXBIND_TEXGEN_UNITS; unit++) {
        uv[unit] = gx_uv_scale(1.0f, 1.0f);
        uint8_t map = p->unit_texmap[unit];
        if (map == GX_TEV_NONE || map >= GXB_MAX_TEXMAPS) {
            bind_unused(unit);
            continue;
        }
        const GxbTexObj* obj = &st->texmap[map];
        C3D_Tex* copy = (C3D_Tex*)gxe_texture_for(obj->data, obj->width, obj->height, &uv[unit]);
        if (copy != NULL) { /* the game's EFB copy buffer (gx_efb.h) */
            GPU_TEXTURE_FILTER_PARAM f = filter_mode(obj->mag_filter);
            gxbind_texture(unit, copy, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE, f, f);
            counts->efb_copy_draws++;
            continue;
        }
        const GxbTlut* tlut = obj->is_ci && obj->tlut_name < GXB_MAX_TLUTS ? &st->tlut[obj->tlut_name] : NULL;
        const GxtcBinding* b = obj->format == GX_TF_C14X2 ? NULL : gxtc_get(obj, tlut);
        if (b == NULL) {
            counts->texture_failures++;
            gxbind_texture(unit, B.fallback, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE, GPU_NEAREST, GPU_NEAREST);
            continue;
        }
        bool sharp = B.sharp && st->projection_type == GXB_PROJ_PERSPECTIVE;
        gxbind_texture(unit, b->tex, wrap_mode(obj->wrap_s), wrap_mode(obj->wrap_t),
                       sharp ? GPU_NEAREST : filter_mode(obj->mag_filter), sharp ? GPU_NEAREST : filter_mode(obj->min_filter));
        uv[unit] = gx_uv_scale(b->u_scale, b->v_scale);
    }
}
