#ifndef AC3DS_GX_STATE_H
#define AC3DS_GX_STATE_H

/* GX state captured by gx_api.c (against the decomp's dolphin/gx.h) and drawn
 * by gx_render.c (libctru + citro3d). The halves never include each other's
 * headers, so this one uses plain C types; numeric values are the GX ones. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_tev.h"

#define GXB_MAX_TEXMAPS  8
#define GXB_MAX_TLUTS    16
#define GXB_MAX_LIGHTS   8
#define GXB_MAX_TEXGENS  8
#define GXB_POS_MATRICES 10 /* GX_PNMTX0..9 (id / 3) */
#define GXB_TEX_MATRICES 10 /* GX_TEXMTX0..9 ((id - 30) / 3) */
#define GXB_EFB_WIDTH    640.0f /* GX viewport, scissor and copy space */
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

/* GX's colour bytes as 0xRRGGBBAA, the form the state keeps. */
static inline uint32_t gxb_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

typedef struct {
    float m[3][4];
} GxbMtx34;

/* A texture object as GX knows it (texel data still in GX layout). */
typedef struct {
    const void* data;
    uint16_t width;
    uint16_t height;
    uint8_t format; /* GXTexFmt / GXCITexFmt */
    bool is_ci;
    uint8_t tlut_name;
    uint8_t wrap_s;    /* GXTexWrapMode */
    uint8_t wrap_t;
    uint8_t min_filter; /* GXTexFilter */
    uint8_t mag_filter;
    bool valid;
} GxbTexObj;

typedef struct {
    const void* data;
    uint8_t format; /* GXTlutFmt */
    uint16_t count;
    bool big_endian; /* cleared by pc_gx_tlut_set_native_le */
    bool valid;
} GxbTlut;

typedef struct {
    float pos[3];   /* eye space */
    uint32_t color; /* 0xRRGGBBAA */
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
    uint8_t projection_mark; /* GxWideMark when it was set (gx_screen.h) */
    GxbMtx34 pos_mtx[GXB_POS_MATRICES];
    GxbMtx34 nrm_mtx[GXB_POS_MATRICES];
    GxbMtx34 tex_mtx[GXB_TEX_MATRICES];
    uint32_t current_mtx; /* GX_PNMTXn id */
    struct {
        uint8_t src;
        uint32_t mtx; /* GX_TEXMTXn id or GX_IDENTITY */
    } texgen[GXB_MAX_TEXGENS];

    /* lighting: channel 0 colour and alpha */
    GxbChannel chan[2];
    uint32_t amb_color; /* 0xRRGGBBAA, COLOR0A0 */
    uint32_t mat_color;
    GxbLight lights[GXB_MAX_LIGHTS];

    /* texture environment (reg/konst colours live in tev) */
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

    /* viewport and scissor in EFB pixels (640 x 480, origin top-left) */
    float viewport[4]; /* left, top, width, height */
    uint8_t viewport_mark; /* GxWideMark when it was set */
    uint32_t scissor[4]; /* left, top, width, height */

    /* fog (GXSetFog), applied after the TEV as on GX */
    uint8_t fog_type; /* GXFogType */
    float fog_start, fog_end, fog_near, fog_far;
    uint32_t fog_color; /* 0xRRGGBBAA */

    GxbTexObj texmap[GXB_MAX_TEXMAPS];
    GxbTlut tlut[GXB_MAX_TLUTS];

    /* vertex descriptor: which attributes each vertex carries */
    bool has_normal;
    bool has_color;
    bool has_texcoord;

    /* Change stamps (gx_api.c): a setter that changes a group gives its stamp a
     * never-used value, so "same as the last draw" is one compare.
     * 0: unknown, treated as changed; a copy changed by hand, not through
     * gx_api.c, must zero its stamps before it is drawn. */
    struct {
        uint64_t projection;
        uint64_t matrices;   /* pos_mtx, nrm_mtx, current_mtx */
        uint64_t texgen;     /* texgen, tex_mtx */
        uint64_t lighting;   /* chan, lights, mat_color, amb_color */
        uint64_t tev;
    } stamps;
} GxbState;

#endif /* AC3DS_GX_STATE_H */
