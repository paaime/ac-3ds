/* gx_efb.c - EFB -> texture copies on the GPU (see gx_efb.h). Colour buffer and
 * texture share the PICA's 8x8-tile layout, so the copy is a raw GX_TextureCopy of
 * whole tile rows: each 240-pixel tile row of the framebuffer lands at the start of
 * a 256-pixel tile row of the texture. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>

#include "gx_efb.h"
#include "gx_render.h"
#include "gx_state.h"

/* game/trace_3ds.c */
void trace3ds_mark(const char* stage);

#define COPY_TEX_WIDTH  256 /* power-of-two texture holding the 240 x 400 frame */
#define COPY_TEX_HEIGHT 512
#define TILE            8
#define BYTES_PER_PIXEL 4 /* GPU_RB_RGBA8 colour buffer, GPU_RGBA8 texture */
#define DIM_UNIT        16 /* GX_TextureCopy line widths and gaps count 16-byte units */
#define RAW_COPY        GX_TRANSFER_RAW_COPY(1)

static struct {
    void* color_buffer;
    uint16_t fb_width, fb_height;
    C3D_Tex tex;
    bool tex_ready;
    const void* dest; /* the game buffer the copy stands for */
    uint16_t copy_width, copy_height;
    float src[4]; /* copied EFB rectangle, as fractions of the EFB */
    uint32_t copies;
    uint32_t copies_this_frame;
    bool warned_cap;
    float image[4];      /* left, top, width, height */
    float copy_image[4]; /* at copy time: a Picture shape switch mid-menu doesn't move the backdrop */
} E;

void gxe_set_source(void* color_buffer, uint16_t width, uint16_t height)
{
    E.color_buffer = color_buffer;
    E.fb_width = width;
    E.fb_height = height;
    gxe_set_image_area(0.0f, 0.0f, 1.0f, 1.0f);
}

void gxe_set_image_area(float left, float top, float width, float height)
{
    E.image[0] = left;
    E.image[1] = top;
    E.image[2] = width;
    E.image[3] = height;
}

void gxe_shutdown(void)
{
    if (E.tex_ready) {
        C3D_TexDelete(&E.tex);
        E.tex_ready = false;
    }
    E.dest = NULL;
}

/* In linear memory, cleared once: the copy fills only 240 x 400 of it, and
 * filtering or coordinates past that edge must not sample leftovers. */
static bool ensure_texture(void)
{
    if (E.tex_ready) {
        return true;
    }
    E.tex_ready = C3D_TexInit(&E.tex, COPY_TEX_WIDTH, COPY_TEX_HEIGHT, GPU_RGBA8);
    if (!E.tex_ready) {
        fprintf(stderr, "[gx] EFB copy: no memory for a %dx%d texture\n", COPY_TEX_WIDTH, COPY_TEX_HEIGHT);
        return false;
    }
    memset(E.tex.data, 0, E.tex.size);
    C3D_TexFlush(&E.tex);
    return true;
}

void gxe_frame_begin(void)
{
    E.copies_this_frame = 0;
}

static bool under_frame_cap(void)
{
    if (E.copies_this_frame < GXE_MAX_COPIES_PER_FRAME) {
        return true;
    }
    if (!E.warned_cap) {
        E.warned_cap = true;
        fprintf(stderr, "[gx] EFB copy: more than %d copies in one frame; the rest are skipped\n",
                GXE_MAX_COPIES_PER_FRAME);
    }
    return false;
}

bool gxe_copy(const void* dest, const uint16_t src[4], uint16_t width, uint16_t height)
{
    if (E.color_buffer == NULL || dest == NULL || width == 0 || height == 0 || src[2] == 0 || src[3] == 0 ||
        E.fb_width > COPY_TEX_WIDTH || E.fb_height > COPY_TEX_HEIGHT || !under_frame_cap() ||
        !ensure_texture()) {
        return false;
    }
    u32 line_bytes = (u32)E.fb_width * TILE * BYTES_PER_PIXEL;
    u32 gap_bytes = (u32)(COPY_TEX_WIDTH - E.fb_width) * TILE * BYTES_PER_PIXEL;
    u32 total = line_bytes * (E.fb_height / TILE);
    u32 in_dim = GX_BUFFER_DIM(line_bytes / DIM_UNIT, 0);
    u32 out_dim = GX_BUFFER_DIM(line_bytes / DIM_UNIT, gap_bytes / DIM_UNIT);
    if (gxr_in_frame()) {
        /* C3D_SyncTextureCopy's in-frame path plus GX_CMDLIST_FLUSH: with the fast
         * frame flush (frame_3ds.c) nothing else flushes these commands. */
        trace3ds_mark("EFB copy: waiting for the GPU (C3D_SyncTextureCopy)");
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
        GX_TextureCopy((u32*)E.color_buffer, in_dim, (u32*)E.tex.data, out_dim, total, RAW_COPY);
    } else {
        C3D_SyncTextureCopy((u32*)E.color_buffer, in_dim, (u32*)E.tex.data, out_dim, total, RAW_COPY);
        trace3ds_mark("game: drawing the frame (emu64 -> GX -> citro3d)");
    }
    E.dest = dest;
    E.copy_width = width;
    E.copy_height = height;
    E.src[0] = src[0] / GXB_EFB_WIDTH;
    E.src[1] = src[1] / GXB_EFB_HEIGHT;
    E.src[2] = src[2] / GXB_EFB_WIDTH;
    E.src[3] = src[3] / GXB_EFB_HEIGHT;
    memcpy(E.copy_image, E.image, sizeof(E.image));
    E.copies++;
    E.copies_this_frame++;
    return true;
}

const void* gxe_texture_for(const void* data, uint16_t width, uint16_t height, GxUvAffine* uv)
{
    /* Size too: a buffer reused for something else isn't the copy. */
    if (E.dest == NULL || data != E.dest || width != E.copy_width || height != E.copy_height || !E.tex_ready) {
        return NULL;
    }
    GxUvCopyLayout layout = { E.fb_width, E.fb_height, COPY_TEX_WIDTH, COPY_TEX_HEIGHT,
                              E.src[0], E.src[1], E.src[2], E.src[3],
                              E.copy_image[0], E.copy_image[1], E.copy_image[2], E.copy_image[3] };
    *uv = gx_uv_rotated_copy(layout);
    return &E.tex;
}

uint32_t gxe_copy_count(void)
{
    return E.copies;
}
