#ifndef AC3DS_GX_EFB_H
#define AC3DS_GX_EFB_H

/* GXCopyTex on the GPU (gx_efb.c): the top screen's framebuffer goes into a texture,
 * which draws binding the game's destination buffer sample instead (as the PC port
 * substitutes a GL texture); the game's buffer itself isn't written. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_uv.h"

/* Colour buffer (RGBA8, tiled) and portrait size (240 x 400 for the top screen). */
void gxe_set_source(void* color_buffer, uint16_t width, uint16_t height);
/* Where the EFB lands on the screen (landscape), as fractions of it. Each frame. */
void gxe_set_image_area(float left, float top, float width, float height);
void gxe_shutdown(void);

/* Each copy takes two of citro3d's 32 GPU queue entries; a full queue panics (svcBreak). */
#define GXE_MAX_COPIES_PER_FRAME 4

void gxe_frame_begin(void);

/* `src`: the copied EFB rectangle (left, top, width, height in 640x480 pixels).
 * Outside a frame it copies the last finished frame; a new destination replaces the
 * last. False if it can't be done (no target, no memory, too many this frame). */
bool gxe_copy(const void* dest, const uint16_t src[4], uint16_t width, uint16_t height);

/* If `data` is the last copy's destination at the copy's size: its texture (a
 * C3D_Tex*) to bind clamped, with the mapping onto it in `uv`. Else NULL. */
const void* gxe_texture_for(const void* data, uint16_t width, uint16_t height, GxUvAffine* uv);

uint32_t gxe_copy_count(void);

#endif /* AC3DS_GX_EFB_H */
