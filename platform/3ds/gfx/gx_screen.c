#include "gx_screen.h"

#include <math.h>

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static uint32_t to_pixels(float fraction, uint32_t size)
{
    return (uint32_t)lroundf(fraction * (float)size);
}

#define FOUR_THREE (4.0f / 3.0f)

GxScreenTarget gx_screen_target(float efb_width, float efb_height, uint32_t width, uint32_t height, bool four_three)
{
    GxScreenTarget t = { efb_width, efb_height, height, width, 0.0f, 0.0f, (float)width, (float)height };
    float pillar_w = (float)height * FOUR_THREE;
    if (four_three && pillar_w < (float)width) {
        t.image_w = pillar_w;
        t.image_x = ((float)width - pillar_w) * 0.5f;
    }
    return t;
}

PicaRect gx_screen_to_pica(GxScreenRect r, GxScreenTarget t)
{
    float x0 = clamp01(r.left / t.efb_width);
    float x1 = clamp01((r.left + r.width) / t.efb_width);
    float y0 = clamp01(r.top / t.efb_height);
    float y1 = clamp01((r.top + r.height) / t.efb_height);
    /* Portrait target: screen y runs down from viewport x = fb_width, screen x right from y = fb_height. */
    float sx0 = t.image_x + x0 * t.image_w;
    float sx1 = t.image_x + x1 * t.image_w;
    float sy0 = t.image_y + y0 * t.image_h;
    float sy1 = t.image_y + y1 * t.image_h;
    uint32_t vx0 = to_pixels(1.0f - sy1 / (float)t.fb_width, t.fb_width);
    uint32_t vx1 = to_pixels(1.0f - sy0 / (float)t.fb_width, t.fb_width);
    uint32_t vy0 = to_pixels(1.0f - sx1 / (float)t.fb_height, t.fb_height);
    uint32_t vy1 = to_pixels(1.0f - sx0 / (float)t.fb_height, t.fb_height);
    PicaRect p = { vx0, vy0, vx1 > vx0 ? vx1 - vx0 : 0, vy1 > vy0 ? vy1 - vy0 : 0 };
    return p;
}

bool gx_screen_is_full(GxScreenRect r, GxScreenTarget t)
{
    PicaRect p = gx_screen_to_pica(r, t);
    return p.x == 0 && p.y == 0 && p.w == t.fb_width && p.h == t.fb_height;
}

GxViewportFold gx_screen_viewport_fold(GxScreenRect v, GxScreenTarget t)
{
    float centre_x = v.left + v.width * 0.5f;
    float centre_y = v.top + v.height * 0.5f;
    float kx = t.image_w / (float)t.fb_height;
    float ky = t.image_h / (float)t.fb_width;
    float cx = 2.0f * (t.image_x + t.image_w * 0.5f) / (float)t.fb_height - 1.0f;
    float cy = 1.0f - 2.0f * (t.image_y + t.image_h * 0.5f) / (float)t.fb_width; /* screen y down, NDC y up */
    GxViewportFold f = {
        kx * (v.width / t.efb_width),
        kx * (2.0f * centre_x / t.efb_width - 1.0f) + cx,
        ky * (v.height / t.efb_height),
        ky * (1.0f - 2.0f * centre_y / t.efb_height) + cy,
        1.0f,
    };
    return f;
}

GxScreenRect gx_screen_intersect(GxScreenRect a, GxScreenRect b)
{
    float left = fmaxf(a.left, b.left);
    float top = fmaxf(a.top, b.top);
    float right = fminf(a.left + a.width, b.left + b.width);
    float bottom = fminf(a.top + a.height, b.top + b.height);
    GxScreenRect r = { left, top, fmaxf(right - left, 0.0f), fmaxf(bottom - top, 0.0f) };
    return r;
}

float gx_screen_wide_factor(GxScreenTarget t)
{
    float factor = (t.efb_width / t.efb_height) / (t.image_w / t.image_h);
    return factor < 1.0f ? factor : 1.0f;
}

float gx_screen_projection_widen(GxWideMark mark, bool orthographic, float factor)
{
    bool widened = mark == GX_WIDE_MORE_VIEW || (mark == GX_WIDE_UI && orthographic);
    return widened ? factor : 1.0f;
}

GxScreenRect gx_screen_ui_viewport(GxScreenRect v, float factor, float efb_width, float efb_height)
{
    /* the whole EFB within a pixel's rounding, as the PC port tests it */
    bool whole = v.left < 1.0f && v.top < 1.0f && v.width > efb_width - 1.0f && v.height > efb_height - 1.0f;
    if (!whole) {
        v.left = (1.0f - factor) * 0.5f * efb_width + v.left * factor;
        v.width *= factor;
    }
    return v;
}
