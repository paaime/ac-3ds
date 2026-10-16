#ifndef AC3DS_GX_FOG_H
#define AC3DS_GX_FOG_H

#include <stdbool.h>
#include <stdint.h>

#define GX_FOG_LUT_SIZE   128
#define GX_FOG_LUT_FLOATS (2 * GX_FOG_LUT_SIZE)

/* GXFogType values: the low three bits pick the curve, bit 3 orthographic. */
enum {
    GX_FOG_TYPE_NONE = 0,
    GX_FOG_TYPE_PERSP_LIN = 2,
    GX_FOG_TYPE_ORTHO_LIN = 10,
};

typedef struct {
    uint8_t type;
    float start, end;
    float near_z, far_z;
} GxFogParams;

bool gx_fog_enabled(uint8_t type);

/* Visibility (1 = clear) at PICA depth `depth` (1 near, 0 far with citro3d's default depth map). */
float gx_fog_visibility(GxFogParams p, float depth);

/* data[0..127]: visibility at depth i / 128; data[128 + i]: data[i + 1] - data[i]. */
void gx_fog_lut(GxFogParams p, float data[GX_FOG_LUT_FLOATS]);

#endif /* AC3DS_GX_FOG_H */
