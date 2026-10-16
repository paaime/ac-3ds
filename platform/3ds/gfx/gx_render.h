#ifndef AC3DS_GX_RENDER_H
#define AC3DS_GX_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    uint32_t draws;
    uint32_t vertices;
    uint32_t skipped_primitives;
    uint32_t tev_fallbacks;
    uint32_t texture_uploads;
    uint32_t texture_entries;
    uint32_t texture_bytes;
    uint32_t vertex_overflow;
    uint32_t texture_failures;
    uint32_t dropped_draws;
    uint32_t max_draw_cmd_words;
    uint32_t efb_copy_draws;
    uint32_t fog_tables;
    uint64_t texture_ticks; /* system ticks */
    uint32_t cmd_words;
} GxrStats;

/* Needs C3D_Init first. */
bool gxr_init(void);
void gxr_shutdown(void);

/* After C3D_FrameDrawOn(target); the target's landscape size. */
void gxr_frame_begin(float target_width, float target_height);
/* Call before C3D_FrameEnd. */
void gxr_frame_end(void);
/* Call after anything else changed citro3d state. */
void gxr_invalidate_state(void);
/* After C3D_FrameDrawOn(another target) within the frame; landscape size. */
void gxr_set_target(float target_width, float target_height);

/* For the next `frames` drawing frames, each draw is logged and must finish within 2 s. */
void gxr_gpu_check(uint32_t frames);

/* Perspective draws only; kept across gxr_init. */
void gxr_set_sharp_textures(bool on);

/* Top screen, from the next frame; kept across gxr_init. */
typedef enum {
    GXR_PICTURE_WIDE,
    GXR_PICTURE_4_3,
    GXR_PICTURE_STRETCH,
} GxrPictureShape;
void gxr_set_picture_shape(GxrPictureShape shape);
/* At the start of a top-screen frame, outside a GX batch. */
void gxr_draw_bars(void);
/* After C3D_FrameEnd; false if the frame's end didn't finish within 2 s. */
bool gxr_gpu_check_frame_end(void);
/* Inside a frame; false if the draws so far didn't finish within 2 s. */
bool gxr_gpu_sync(void);

bool gxr_in_frame(void);

/* `texture` is a C3D_Tex*; top-screen pixels (400 x 240, origin top-left); inside a frame only. */
void gxr_blit(const void* texture, float x, float y, float w, float h, float s1, float t1);

void gxr_begin(uint8_t primitive, uint16_t vertex_count);
void gxr_position(float x, float y, float z);
void gxr_normal(float x, float y, float z);
void gxr_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void gxr_texcoord(float s, float t);
void gxr_end(const GxbState* state);
bool gxr_batch_open(void);

/* 0xRRGGBBAA */
void gxr_set_clear_color(uint32_t rgba);
uint32_t gxr_clear_color(void);

GxrStats gxr_stats(void);

#endif /* AC3DS_GX_RENDER_H */
