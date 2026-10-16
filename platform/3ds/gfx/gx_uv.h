#ifndef AC3DS_GX_UV_H
#define AC3DS_GX_UV_H

typedef struct {
    float m[2][3]; /* (s', t') = m * (s, t, 1) */
} GxUvAffine;

typedef struct {
    float fb_width, fb_height; /* copied framebuffer, portrait (240 x 400) */
    float tex_width, tex_height;
    /* GXSetTexCopySrc's rectangle, as fractions of the EFB */
    float src_left, src_top, src_width, src_height;
    /* where the EFB lands on the landscape screen, as fractions of it */
    float image_left, image_top, image_width, image_height;
} GxUvCopyLayout;

GxUvAffine gx_uv_scale(float u, float v);

void gx_uv_apply(GxUvAffine a, const float row0[4], const float row1[4], float out0[4], float out1[4]);

GxUvAffine gx_uv_rotated_copy(GxUvCopyLayout layout);

#endif /* AC3DS_GX_UV_H */
