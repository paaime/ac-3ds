/* nes_video_3ds.c - the NES picture (see nes_video_3ds.h), tiled each frame into
 * a 256x256 texture (rows flipped, t = 0 at the top) and blitted. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>

#include "gx_render.h"
#include "nes_video_3ds.h"
#include "pica_tile.h"

#define NES_WIDTH       256
#define NES_LINES       240
#define CROP_TOP        8   /* as pc_nes_fixnes.c: the top and bottom 8 lines are overscan */
#define SHOWN_LINES     224
#define TEX_SIZE        256
#define TEXEL_BYTES     2
#define SCREEN_WIDTH    400
#define SCREEN_HEIGHT   240
#define TILE            8
/* With the row flip, the shown lines fill the texture's last rows. */
#define SHOWN_OFFSET_BYTES ((size_t)(TEX_SIZE - SHOWN_LINES) * TEX_SIZE * TEXEL_BYTES)

static C3D_Tex s_tex;
static bool s_ready;
static bool s_failed;

static bool ensure_texture(void)
{
    if (s_ready || s_failed) {
        return s_ready;
    }
    s_ready = C3D_TexInit(&s_tex, TEX_SIZE, TEX_SIZE, GPU_RGB565);
    s_failed = !s_ready;
    if (s_failed) {
        fprintf(stderr, "[nes] no memory for the NES picture's texture\n");
        return false;
    }
    memset(s_tex.data, 0, s_tex.size);
    return true;
}

void nes_video_show(const uint16_t* frame)
{
    if (frame == NULL || !ensure_texture()) {
        return;
    }
    _Static_assert((TEX_SIZE - SHOWN_LINES) % TILE == 0, "whole tile rows");
    pica_tile((uint8_t*)s_tex.data + SHOWN_OFFSET_BYTES, frame + (size_t)NES_WIDTH * CROP_TOP, TEX_SIZE,
              SHOWN_LINES, TEXEL_BYTES, true);
    C3D_TexFlush(&s_tex);
    float x = (SCREEN_WIDTH - NES_WIDTH) / 2.0f;
    float y = (SCREEN_HEIGHT - SHOWN_LINES) / 2.0f;
    gxr_blit(&s_tex, x, y, NES_WIDTH, SHOWN_LINES, 1.0f, (float)SHOWN_LINES / TEX_SIZE);
}

void nes_video_release(void)
{
    if (s_ready) {
        C3D_TexDelete(&s_tex);
        s_ready = false;
    }
    s_failed = false;
}
