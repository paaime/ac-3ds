#ifndef AC3DS_GX_STATE_H
#define AC3DS_GX_STATE_H

/* Shared by the dolphin/gx.h and citro3d halves, which never include each other: plain C types, GX values. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_tev.h"

#define GXB_MAX_TEXMAPS  8
#define GXB_MAX_TLUTS    16
#define GXB_MAX_LIGHTS   8
#define GXB_MAX_TEXGENS  8
#define GXB_POS_MATRICES 10 /* GX_PNMTX0..9 (id / 3) */
#define GXB_TEX_MATRICES 10 /* GX_TEXMTX0..9 ((id - 30) / 3) */
#define GXB_EFB_WIDTH    640.0f
#define GXB_EFB_HEIGHT   480.0f

enum {
    GXB_PROJ_PERSPECTIVE = 0,
    GXB_PROJ_ORTHOGRAPHIC = 1,
    GXB_TEXMTX_BASE = 30,
    GXB_TEXMTX_IDENTITY = 60,
    GXB_TG_SRC_TEX0 = 4,
    GXB_CULL_NONE = 0,
    GXB_CULL_FRONT = 1,
    GXB_CULL_BACK = 2,
    GXB_CULL_ALL = 3,
    GXB_BM_NONE = 0,
    GXB_BM_BLEND = 1,
    GXB_BM_LOGIC = 2,
    GXB_BM_SUBTRACT = 3,
    GXB_SRC_REG = 0,
    GXB_SRC_VTX = 1,
    GXB_CHAN_COLOR0 = 0,
    GXB_CHAN_ALPHA0 = 1,
};

/* 0xRRGGBBAA, the form every colour in the state takes. */
static inline uint32_t gxb_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

typedef struct {
    float m[3][4];
} GxbMtx34;

typedef struct {
    const void* data;
    uint16_t width;
    uint16_t height;
    uint8_t format;
    bool is_ci;
    uint8_t tlut_name;
    uint8_t wrap_s;
    uint8_t wrap_t;
    uint8_t min_filter;
    uint8_t mag_filter;
    bool valid;
} GxbTexObj;

typedef struct {
    const void* data;
    uint8_t format;
    uint16_t count;
    bool big_endian; /* cleared by pc_gx_tlut_set_native_le */
    bool valid;
} GxbTlut;

typedef struct {
    float pos[3]; /* eye space */
    uint32_t color;
} GxbLight;

typedef struct {
    bool enable;
    uint8_t amb_src;
    uint8_t mat_src;
    uint32_t light_mask;
} GxbChannel;

typedef struct {
    float projection[4][4];
    uint8_t projection_type;
    uint8_t projection_mark;
    GxbMtx34 pos_mtx[GXB_POS_MATRICES];
    GxbMtx34 nrm_mtx[GXB_POS_MATRICES];
    GxbMtx34 tex_mtx[GXB_TEX_MATRICES];
    uint32_t current_mtx;
    struct {
        uint8_t src;
        uint32_t mtx;
    } texgen[GXB_MAX_TEXGENS];

    GxbChannel chan[2];
    uint32_t amb_color;
    uint32_t mat_color;
    GxbLight lights[GXB_MAX_LIGHTS];

    GxTevConfig tev;

    uint8_t blend_type;
    uint8_t blend_src;
    uint8_t blend_dst;
    bool color_update;
    bool alpha_update;
    bool z_enable;
    uint8_t z_func;
    bool z_update;
    uint8_t alpha_comp0;
    uint8_t alpha_ref0;
    uint8_t alpha_op;
    uint8_t alpha_comp1;
    uint8_t alpha_ref1;
    uint8_t cull_mode;

    /* left, top, width, height in EFB pixels (640 x 480, origin top-left) */
    float viewport[4];
    uint8_t viewport_mark;
    uint32_t scissor[4];

    uint8_t fog_type;
    float fog_start, fog_end, fog_near, fog_far;
    uint32_t fog_color;

    GxbTexObj texmap[GXB_MAX_TEXMAPS];
    GxbTlut tlut[GXB_MAX_TLUTS];

    bool has_normal;
    bool has_color;
    bool has_texcoord;

    /* 0 means changed: zero them in a copy changed by hand, not through gx_api.c, before drawing it. */
    struct {
        uint64_t projection;
        uint64_t matrices;
        uint64_t texgen;
        uint64_t lighting;
        uint64_t tev;
    } stamps;
} GxbState;

#endif /* AC3DS_GX_STATE_H */
