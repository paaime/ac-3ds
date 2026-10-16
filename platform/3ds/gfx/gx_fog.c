#include "gx_fog.h"

#include <math.h>

#define CURVE_MASK 7u
#define ORTHO_BIT  8u
#define EXP_STEEPNESS 8.0f /* GX's curves are powers of 2^-8 */

enum { CURVE_LIN = 2, CURVE_EXP = 4, CURVE_EXP2 = 5, CURVE_REVEXP = 6, CURVE_REVEXP2 = 7 };

bool gx_fog_enabled(uint8_t type)
{
    uint8_t curve = type & CURVE_MASK;
    return curve == CURVE_LIN || curve >= CURVE_EXP;
}

static float distance(GxFogParams p, float depth)
{
    float span = p.far_z - p.near_z;
    if (p.type & ORTHO_BIT) {
        return p.far_z - depth * span;
    }
    float denom = depth * span + p.near_z; /* citro3d's FogLut_CalcZ */
    return denom > 0.0f ? p.far_z * p.near_z / denom : p.far_z;
}

static float ramp(GxFogParams p, float dist)
{
    float span = p.end - p.start;
    if (span == 0.0f) {
        return dist >= p.end ? 1.0f : 0.0f;
    }
    float z = (dist - p.start) / span;
    return z < 0.0f ? 0.0f : (z > 1.0f ? 1.0f : z);
}

static float fog_amount(uint8_t curve, float z)
{
    switch (curve) {
    case CURVE_EXP: return 1.0f - exp2f(-EXP_STEEPNESS * z);
    case CURVE_EXP2: return 1.0f - exp2f(-EXP_STEEPNESS * z * z);
    case CURVE_REVEXP: return exp2f(-EXP_STEEPNESS * (1.0f - z));
    case CURVE_REVEXP2: return exp2f(-EXP_STEEPNESS * (1.0f - z) * (1.0f - z));
    default: return z;
    }
}

float gx_fog_visibility(GxFogParams p, float depth)
{
    if (!gx_fog_enabled(p.type)) {
        return 1.0f;
    }
    float v = 1.0f - fog_amount(p.type & CURVE_MASK, ramp(p, distance(p, depth)));
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

void gx_fog_lut(GxFogParams p, float data[GX_FOG_LUT_FLOATS])
{
    float prev = gx_fog_visibility(p, 0.0f);
    for (int i = 0; i < GX_FOG_LUT_SIZE; i++) {
        float next = gx_fog_visibility(p, (float)(i + 1) / GX_FOG_LUT_SIZE);
        data[i] = prev;
        data[GX_FOG_LUT_SIZE + i] = next - prev;
        prev = next;
    }
}
