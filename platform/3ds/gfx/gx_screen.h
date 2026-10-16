#ifndef AC3DS_GX_SCREEN_H
#define AC3DS_GX_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float left, top, width, height; /* EFB pixels */
} GxScreenRect;

typedef struct {
    float efb_width, efb_height;
    uint32_t fb_width, fb_height; /* portrait: 240 x 400 */
    /* where the EFB lands, in landscape screen pixels */
    float image_x, image_y, image_w, image_h;
} GxScreenTarget;

/* `width` x `height` is landscape. */
GxScreenTarget gx_screen_target(float efb_width, float efb_height, uint32_t width, uint32_t height, bool four_three);

typedef struct {
    uint32_t x, y, w, h;
} PicaRect;

/* x' = scale_x * x + offset_x * w in clip space, so an off-screen viewport is cropped as on GX. */
typedef struct {
    float scale_x, offset_x;
    float scale_y, offset_y;
    float widen_x;
} GxViewportFold;

/* emu64's widescreen markers (the PC port's g_pc_widescreen_stretch). */
typedef enum {
    GX_WIDE_MORE_VIEW = 0,
    GX_WIDE_STRETCHED = 1,
    GX_WIDE_UI = 2,
} GxWideMark;

/* Clamped to the image area; w or h is 0 if nothing is left. */
PicaRect gx_screen_to_pica(GxScreenRect r, GxScreenTarget t);
bool gx_screen_is_full(GxScreenRect r, GxScreenTarget t);
GxViewportFold gx_screen_viewport_fold(GxScreenRect viewport, GxScreenTarget t);
GxScreenRect gx_screen_intersect(GxScreenRect a, GxScreenRect b);

float gx_screen_wide_factor(GxScreenTarget t);
float gx_screen_projection_widen(GxWideMark mark, bool orthographic, float factor);
GxScreenRect gx_screen_ui_viewport(GxScreenRect v, float factor, float efb_width, float efb_height);

#endif /* AC3DS_GX_SCREEN_H */
