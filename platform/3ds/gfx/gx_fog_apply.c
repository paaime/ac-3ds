/* gx_fog_apply.c - GX fog on citro3d (see gx_fog_apply.h). */
#include "gx_fog_apply.h"

#include <3ds.h>
#include <citro3d.h>

#include "gx_fog.h"

static struct {
    C3D_FogLut lut;
    GxFogParams params;
    bool lut_valid;
    bool on;
    bool on_valid;
    uint32_t color;
    bool color_valid;
} F; /* what was last handed to citro3d */

static bool same_fog(GxFogParams a, GxFogParams b)
{
    return a.type == b.type && a.start == b.start && a.end == b.end && a.near_z == b.near_z && a.far_z == b.far_z;
}

/* 0xRRGGBBAA -> GPUREG_FOG_COLOR (0x00BBGGRR). */
static u32 to_fog_color(uint32_t rgba)
{
    return ((rgba >> 24) & 0xFF) | (((rgba >> 16) & 0xFF) << 8) | (((rgba >> 8) & 0xFF) << 16);
}

bool gxf_apply(const GxbState* st)
{
    bool table_sent = false;
    bool on = gx_fog_enabled(st->fog_type);
    if (on) {
        GxFogParams p = { st->fog_type, st->fog_start, st->fog_end, st->fog_near, st->fog_far };
        if (!F.lut_valid || !same_fog(p, F.params)) {
            float data[GX_FOG_LUT_FLOATS];
            gx_fog_lut(p, data);
            FogLut_FromArray(&F.lut, data);
            C3D_FogLutBind(&F.lut);
            F.params = p;
            F.lut_valid = true;
            table_sent = true;
        }
        if (!F.color_valid || st->fog_color != F.color) {
            C3D_FogColor(to_fog_color(st->fog_color));
            F.color = st->fog_color;
            F.color_valid = true;
        }
    }
    if (!F.on_valid || on != F.on) {
        C3D_FogGasMode(on ? GPU_FOG : GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        F.on = on;
        F.on_valid = true;
    }
    return table_sent;
}

void gxf_invalidate(void)
{
    F.lut_valid = false;
    F.on_valid = false;
    F.color_valid = false;
}
