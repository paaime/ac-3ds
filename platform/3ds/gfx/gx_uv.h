#ifndef AC3DS_GX_UV_H
#define AC3DS_GX_UV_H

/* Texture-coordinate adjustments folded into each texgen's matrix rows (the
 * shader computes s = row0 . (s, t, 0, 1), t = row1 . ...): a texture's
 * power-of-two padding scale, and the mapping onto an EFB copy of the rotated
 * portrait framebuffer (gx_efb.c). */

typedef struct {
    float m[2][3]; /* (s', t') = m * (s, t, 1) */
} GxUvAffine;

typedef struct {
    float fb_width, fb_height;   /* copied framebuffer, portrait (240 x 400) */
    float tex_width, tex_height; /* the texture holding it (power of two) */
    /* the part of the EFB the copy covers (GXSetTexCopySrc), as fractions of it */
    float src_left, src_top, src_width, src_height;
    /* where the EFB lands on the screen (landscape), as fractions of it */
    float image_left, image_top, image_width, image_height;
} GxUvCopyLayout;

GxUvAffine gx_uv_scale(float u, float v);

/* rows' = a applied after rows (both 4-vectors over (s, t, 0, 1)). */
void gx_uv_apply(GxUvAffine a, const float row0[4], const float row1[4], float out0[4], float out1[4]);

GxUvAffine gx_uv_rotated_copy(GxUvCopyLayout layout);

#endif /* AC3DS_GX_UV_H */
