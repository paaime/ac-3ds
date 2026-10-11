#ifndef AC3DS_GX_SCREEN_H
#define AC3DS_GX_SCREEN_H

/* GX viewports and scissors (EFB pixels, 640 x 480, origin top-left) on a 3DS render
 * target, and the Wide picture's corrections. Targets are portrait, a quarter-turn: on
 * the top screen (240 x 400) its left edge is viewport y = 400, its right y = 0, its
 * top x = 240 and its bottom x = 0. */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float left, top, width, height; /* EFB pixels */
} GxScreenRect;

typedef struct {
    float efb_width, efb_height; /* 640 x 480 */
    uint32_t fb_width, fb_height; /* portrait: 240 x 400 */
    /* where the EFB lands, in landscape screen pixels (x right, y down) */
    float image_x, image_y, image_w, image_h;
} GxScreenTarget;

/* `width` x `height` is landscape; `four_three` puts the EFB in a 4:3 area in
 * the middle when the target is wider. */
GxScreenTarget gx_screen_target(float efb_width, float efb_height, uint32_t width, uint32_t height, bool four_three);

typedef struct {
    uint32_t x, y, w, h; /* viewport coordinates of the portrait target */
} PicaRect;

/* A GX viewport as a clip-space scale and offset, x' = scale_x * x + offset_x * w,
 * so the PICA viewport stays the whole target and a viewport partly off the
 * screen is cropped, as on GX, not squashed. */
typedef struct {
    float scale_x, offset_x;
    float scale_y, offset_y;
    float widen_x; /* the projection's own x scale times this: below 1 sees a wide screen wider */
} GxViewportFold;

/* How the game marks what it draws for a wide screen (emu64's widescreen
 * markers, the PC port's g_pc_widescreen_stretch): the world, seen wider;
 * full-screen transitions and pictures, stretched; menus, kept 4:3. */
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

/* The x scale that keeps the EFB's proportions on the target's image: 0.8 on the
 * 5:3 top screen, 1 where the image is no wider than the EFB. */
float gx_screen_wide_factor(GxScreenTarget t);
/* The factor for a projection's x scale, as the PC port widens it (1: none). */
float gx_screen_projection_widen(GxWideMark mark, bool orthographic, float factor);
/* A menu's viewport other than the whole EFB, where the narrowed menu puts it:
 * in the 4:3 middle. */
GxScreenRect gx_screen_ui_viewport(GxScreenRect v, float factor, float efb_width, float efb_height);

#endif /* AC3DS_GX_SCREEN_H */
