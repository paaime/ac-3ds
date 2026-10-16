#ifndef AC3DS_GX_UNIFORMS_H
#define AC3DS_GX_UNIFORMS_H

#include <3ds.h>
#include <stdbool.h>

#include "gx_screen.h"
#include "gx_state.h"
#include "gx_tev.h"
#include "gx_uv.h"

#define GXU_TEXGENS GX_TEV_MAX_TEXCOORDS

/* false if the program lacks one of the uniforms. */
bool gxu_locate(shaderInstance_s* vs);

void gxu_upload_draw(const GxbState* st, GxViewportFold fold, const GxUvAffine uv[GXU_TEXGENS]);

void gxu_upload_blit(void);

/* Call when citro3d's uniforms may have changed elsewhere. */
void gxu_invalidate(void);

#endif /* AC3DS_GX_UNIFORMS_H */
