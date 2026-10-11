#ifndef AC3DS_GX_BIND_H
#define AC3DS_GX_BIND_H

/* A draw's textures on the PICA's three units: the texmaps the combiner samples,
 * from the texture cache or the EFB copy, each bound only when it isn't already. */

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"
#include "gx_tev.h"
#include "gx_uv.h"

#define GXBIND_TEXGEN_UNITS GX_TEV_MAX_TEXCOORDS /* the shader's texcoord outputs */

typedef struct {
    uint32_t efb_copy_draws;   /* units that sampled the EFB copy */
    uint32_t texture_failures; /* textures that couldn't be decoded: the fallback instead */
} GxBindCounts;

/* The white texture for what can't be decoded and for unused units (the real
 * GPU hangs on a unit never given one). */
void gxbind_set_fallback(C3D_Tex* fallback);
/* "Sharp textures": unsmoothed sampling for 3D draws. Kept across the renderer's init. */
void gxbind_set_sharp(bool on);

/* Skipped if the unit already has exactly that and no texels changed since.
 * Nothing for a unit the PICA lacks or a NULL texture. */
void gxbind_texture(int unit, C3D_Tex* tex, GPU_TEXTURE_WRAP_PARAM wrap_s, GPU_TEXTURE_WRAP_PARAM wrap_t,
                    GPU_TEXTURE_FILTER_PARAM mag, GPU_TEXTURE_FILTER_PARAM min);
/* A blit's: `tex` on unit 0, unfiltered and clamped, the fallback on the others. */
void gxbind_blit(C3D_Tex* tex);
/* The draw's textures: each texgen unit's uv mapping into `uv`; counts added. */
void gxbind_draw(const GxbState* st, const PicaCombiner* p, GxUvAffine uv[GXBIND_TEXGEN_UNITS],
                 GxBindCounts* counts);
/* citro3d's bindings may have changed: the next draw binds again. */
void gxbind_invalidate(void);

#endif /* AC3DS_GX_BIND_H */
