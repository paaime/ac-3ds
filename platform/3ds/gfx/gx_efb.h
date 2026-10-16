#ifndef AC3DS_GX_EFB_H
#define AC3DS_GX_EFB_H

#include <stdbool.h>
#include <stdint.h>

#include "gx_uv.h"

/* RGBA8 tiled colour buffer, portrait size (240 x 400 for the top screen). */
void gxe_set_source(void* color_buffer, uint16_t width, uint16_t height);
/* Fractions of the landscape screen; set each frame. */
void gxe_set_image_area(float left, float top, float width, float height);
void gxe_shutdown(void);

/* Each copy takes two of citro3d's 32 GPU queue entries; a full queue panics (svcBreak). */
#define GXE_MAX_COPIES_PER_FRAME 4

void gxe_frame_begin(void);

/* `src` in 640x480 EFB pixels; outside a frame it copies the last finished frame. */
bool gxe_copy(const void* dest, const uint16_t src[4], uint16_t width, uint16_t height);

/* A C3D_Tex* to bind clamped if `data` is the last copy's destination at its size, else NULL. */
const void* gxe_texture_for(const void* data, uint16_t width, uint16_t height, GxUvAffine* uv);

uint32_t gxe_copy_count(void);

#endif /* AC3DS_GX_EFB_H */
