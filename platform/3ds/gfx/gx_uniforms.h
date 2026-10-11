#ifndef AC3DS_GX_UNIFORMS_H
#define AC3DS_GX_UNIFORMS_H

/* The vertex shader's uniforms (gx.v.pica), each sent only when its value changes. */

#include <3ds.h>
#include <stdbool.h>

#include "gx_screen.h"
#include "gx_state.h"
#include "gx_tev.h"
#include "gx_uv.h"

#define GXU_TEXGENS GX_TEV_MAX_TEXCOORDS /* the shader's texcoord outputs */

/* The uniforms' registers in `vs`: false if the program lacks one. */
bool gxu_locate(shaderInstance_s* vs);

/* `fold` maps the viewport into clip space and widens the projection's x (widen_x);
 * `uv` is each bound texture's mapping. */
void gxu_upload_draw(const GxbState* st, GxViewportFold fold, const GxUvAffine uv[GXU_TEXGENS]);

/* A blit's: only the screen's quarter-turn, the vertex colour, unlit (gxr_blit). */
void gxu_upload_blit(void);

/* citro3d's values may have changed: the next draw sends everything again. */
void gxu_invalidate(void);

#endif /* AC3DS_GX_UNIFORMS_H */
