#ifndef AC3DS_GX_RENDER_H
#define AC3DS_GX_RENDER_H

/* citro3d side of the GX backend: vertex batches drawn with the captured GX state. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    uint32_t draws;
    uint32_t vertices;
    uint32_t skipped_primitives; /* lines, points */
    uint32_t tev_fallbacks;      /* draws whose TEV config couldn't be translated */
    uint32_t texture_uploads;    /* texture cache misses this frame */
    uint32_t texture_entries;
    uint32_t texture_bytes;      /* linear memory */
    uint32_t vertex_overflow;    /* dropped: the ring was full */
    uint32_t texture_failures;   /* textures that couldn't be decoded (white fallback) */
    uint32_t dropped_draws;      /* the command buffer was full */
    uint32_t max_draw_cmd_words; /* 32-bit words */
    uint32_t efb_copy_draws;
    uint32_t fog_tables;         /* fog tables sent (each costs ~130 command words) */
    uint64_t texture_ticks;      /* system ticks */
    uint32_t cmd_words;
} GxrStats;

/* One-time setup: shader, vertex ring, texture cache. Needs C3D_Init first. */
bool gxr_init(void);
void gxr_shutdown(void);

/* After C3D_FrameDrawOn(target); the size is the target's landscape size. */
void gxr_frame_begin(float target_width, float target_height);
/* Call before C3D_FrameEnd. */
void gxr_frame_end(void);
/* After anything else changed citro3d state: the next draw sends all of its own. */
void gxr_invalidate_state(void);
/* After C3D_FrameDrawOn(another target) within the frame (landscape size). */
void gxr_set_target(float target_width, float target_height);

/* GPU hang diagnostic: in the next `frames` drawing frames every draw is logged
 * (gx_debug.h) and must finish within 2 s; the first that doesn't is reported. */
void gxr_gpu_check(uint32_t frames);

/* "Sharp textures": 3D (perspective) draws sample unsmoothed; 2D ones keep the
 * game's filtering (shown at ~0.6x, unsmoothed text loses strokes). Kept across gxr_init. */
void gxr_set_sharp_textures(bool on);

/* "Picture shape" on the top screen, from the next frame; kept across gxr_init.
 * Wide shows more of the world at the sides (what the game marks as menus and
 * full-screen pictures excepted), 4:3 puts the picture in the middle (320 x
 * 240), Stretch stretches it over the screen. */
typedef enum {
    GXR_PICTURE_WIDE,
    GXR_PICTURE_4_3,
    GXR_PICTURE_STRETCH,
} GxrPictureShape;
void gxr_set_picture_shape(GxrPictureShape shape);
/* At the start of a top-screen frame: black bars at the 4:3 picture's sides.
 * Like a blit: outside a GX batch. */
void gxr_draw_bars(void);
/* After C3D_FrameEnd: while checking, the frame's end must finish too (false if not). */
bool gxr_gpu_check_frame_end(void);
/* Inside a frame: what was drawn so far must finish within 2 s (false if not). */
bool gxr_gpu_sync(void);

bool gxr_in_frame(void);

/* `texture` (a C3D_Tex*) unblended and unfiltered at (x, y, w, h) in top-screen
 * pixels (400 x 240, origin top-left), sampling (0, 0)-(s1, t1). Only inside a frame. */
void gxr_blit(const void* texture, float x, float y, float w, float h, float s1, float t1);

/* Immediate-mode vertex capture (GXBegin / GXPosition / ... / GXEnd). */
void gxr_begin(uint8_t primitive, uint16_t vertex_count);
void gxr_position(float x, float y, float z);
void gxr_normal(float x, float y, float z);
void gxr_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gxr_texcoord(float s, float t);
void gxr_end(const GxbState* state);
bool gxr_batch_open(void);

/* GXSetCopyClear, 0xRRGGBBAA. */
void gxr_set_clear_color(uint32_t rgba);
uint32_t gxr_clear_color(void);

/* Stats of the frame in progress (reset by gxr_frame_begin). */
GxrStats gxr_stats(void);

#endif /* AC3DS_GX_RENDER_H */
