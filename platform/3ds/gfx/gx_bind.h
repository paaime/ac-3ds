#ifndef AC3DS_GX_BIND_H
#define AC3DS_GX_BIND_H

#include <citro3d.h>
#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"
#include "gx_tev.h"
#include "gx_uv.h"

#define GXBIND_TEXGEN_UNITS GX_TEV_MAX_TEXCOORDS

typedef struct {
    uint32_t efb_copy_draws;
    uint32_t texture_failures;
} GxBindCounts;

/* Must be white: it stands in for textures that can't be decoded and fills unused units. */
void gxbind_set_fallback(C3D_Tex* fallback);
/* Nearest sampling for 3D draws; kept across the renderer's init. */
void gxbind_set_sharp(bool on);

void gxbind_texture(int unit, C3D_Tex* tex, GPU_TEXTURE_WRAP_PARAM wrap_s, GPU_TEXTURE_WRAP_PARAM wrap_t,
                    GPU_TEXTURE_FILTER_PARAM mag, GPU_TEXTURE_FILTER_PARAM min);
void gxbind_blit(C3D_Tex* tex);
void gxbind_draw(const GxbState* st, const PicaCombiner* p, GxUvAffine uv[GXBIND_TEXGEN_UNITS],
                 GxBindCounts* counts);
void gxbind_invalidate(void);

#endif /* AC3DS_GX_BIND_H */
