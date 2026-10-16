#include "gx_uv.h"

GxUvAffine gx_uv_scale(float u, float v)
{
    GxUvAffine a = { { { u, 0.0f, 0.0f }, { 0.0f, v, 0.0f } } };
    return a;
}

void gx_uv_apply(GxUvAffine a, const float row0[4], const float row1[4], float out0[4], float out1[4])
{
    for (int i = 0; i < 4; i++) {
        out0[i] = a.m[0][0] * row0[i] + a.m[0][1] * row1[i];
        out1[i] = a.m[1][0] * row0[i] + a.m[1][1] * row1[i];
    }
    out0[3] += a.m[0][2]; /* the constant term rides on the input's w = 1 */
    out1[3] += a.m[1][2];
}

/* Portrait framebuffer: row 0 is the screen's left edge, and PICA samples the first stored row at t = 1. */
GxUvAffine gx_uv_rotated_copy(GxUvCopyLayout l)
{
    float src_left = l.image_left + l.src_left * l.image_width;
    float src_width = l.src_width * l.image_width;
    float src_top = l.image_top + l.src_top * l.image_height;
    float src_height = l.src_height * l.image_height;
    float across = l.fb_width / l.tex_width;
    float along = l.fb_height / l.tex_height;
    GxUvAffine a = { {
        { 0.0f, -across * src_height, across * (1.0f - src_top) },
        { -along * src_width, 0.0f, 1.0f - along * src_left },
    } };
    return a;
}
