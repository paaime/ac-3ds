#ifndef AC3DS_GX_TEV_H
#define AC3DS_GX_TEV_H

/* GX TEV -> PICA200 texture combiner translation. A GX TEV stage computes
 *     out = clamp((d OP lerp(a, b, c) + bias) * scale),  lerp(a, b, c) = a*(1-c) + b*c
 * and a PICA stage func(src0, src1, src2) with one constant per stage plus one
 * global combiner buffer colour. Each TEV stage becomes one or two PICA stages
 * (6 available); emu64's patterns all fit:
 *   b*c -> MODULATE, a*(1-c)+b*c -> INTERPOLATE, b*c+d -> MULTIPLY_ADD, d-x -> SUBTRACT;
 *   a second constant in one stage uses the buffer colour.
 * Enum values equal the SDK/hardware ones, so they pass straight through. */

#include <stdbool.h>
#include <stdint.h>

#define GX_TEV_MAX_STAGES        16
#define PICA_COMBINER_MAX_STAGES 6
#define PICA_TEXTURE_UNITS       3
/* The shader's texcoords (emu64 sets up at most two texgens). PICA unit n
 * samples texcoord n, so this also limits the usable texture units. */
#define GX_TEV_MAX_TEXCOORDS     2
#define GX_TEV_NONE              0xFF /* null texmap / texcoord / channel */

/* GXTevColorArg */
enum {
    GXT_CC_CPREV = 0, GXT_CC_APREV, GXT_CC_C0, GXT_CC_A0, GXT_CC_C1, GXT_CC_A1, GXT_CC_C2, GXT_CC_A2,
    GXT_CC_TEXC, GXT_CC_TEXA, GXT_CC_RASC, GXT_CC_RASA, GXT_CC_ONE, GXT_CC_HALF, GXT_CC_KONST, GXT_CC_ZERO,
};
/* GXTevAlphaArg */
enum {
    GXT_CA_APREV = 0, GXT_CA_A0, GXT_CA_A1, GXT_CA_A2, GXT_CA_TEXA, GXT_CA_RASA, GXT_CA_KONST, GXT_CA_ZERO,
};
/* GXTevOp, GXTevBias, GXTevScale, GXTevRegID */
enum { GXT_OP_ADD = 0, GXT_OP_SUB = 1 };
enum { GXT_BIAS_ZERO = 0, GXT_BIAS_ADDHALF = 1, GXT_BIAS_SUBHALF = 2 };
enum { GXT_SCALE_1 = 0, GXT_SCALE_2 = 1, GXT_SCALE_4 = 2, GXT_SCALE_HALF = 3 };
enum { GXT_REG_PREV = 0, GXT_REG_0, GXT_REG_1, GXT_REG_2 };

typedef struct {
    uint8_t in[4]; /* a, b, c, d */
    uint8_t op;
    uint8_t bias;
    uint8_t scale;
    bool clamp;
    uint8_t out_reg;
} GxTevChannel;

typedef struct {
    GxTevChannel color;
    GxTevChannel alpha;
    uint8_t tex_coord;  /* GX_TEXCOORDn or GX_TEV_NONE */
    uint8_t tex_map;    /* GX_TEXMAPn or GX_TEV_NONE */
    uint8_t kcolor_sel; /* GXTevKColorSel */
    uint8_t kalpha_sel; /* GXTevKAlphaSel */
} GxTevStage;

/* Colours are 0xRRGGBBAA. */
typedef struct {
    int stage_count;
    GxTevStage stages[GX_TEV_MAX_STAGES];
    uint32_t reg[4];   /* PREV, REG0, REG1, REG2 initial values (GXSetTevColor) */
    uint32_t konst[4]; /* KONST0..3 (GXSetTevKColor) */
} GxTevConfig;

/* GPU_TEVSRC */
enum {
    PICA_SRC_PRIMARY = 0x0, PICA_SRC_TEXTURE0 = 0x3, PICA_SRC_TEXTURE1 = 0x4, PICA_SRC_TEXTURE2 = 0x5,
    PICA_SRC_PREVIOUS_BUFFER = 0xD, PICA_SRC_CONSTANT = 0xE, PICA_SRC_PREVIOUS = 0xF,
};
/* GPU_TEVOP_RGB / GPU_TEVOP_A */
enum { PICA_OP_RGB_COLOR = 0, PICA_OP_RGB_ONE_MINUS_COLOR = 1, PICA_OP_RGB_ALPHA = 2, PICA_OP_RGB_ONE_MINUS_ALPHA = 3 };
enum { PICA_OP_A_ALPHA = 0, PICA_OP_A_ONE_MINUS_ALPHA = 1 };
/* GPU_COMBINEFUNC */
enum {
    PICA_FUNC_REPLACE = 0, PICA_FUNC_MODULATE = 1, PICA_FUNC_ADD = 2, PICA_FUNC_ADD_SIGNED = 3,
    PICA_FUNC_INTERPOLATE = 4, PICA_FUNC_SUBTRACT = 5, PICA_FUNC_MULTIPLY_ADD = 8,
};
/* GPU_TEVSCALE */
enum { PICA_SCALE_1 = 0, PICA_SCALE_2 = 1, PICA_SCALE_4 = 2 };

typedef struct {
    uint8_t func;
    uint8_t src[3];
    uint8_t op[3];
    uint8_t scale;
} PicaCombinerOp;

typedef struct {
    PicaCombinerOp rgb;
    PicaCombinerOp alpha;
    uint32_t constant; /* 0xRRGGBBAA */
} PicaCombinerStage;

typedef struct {
    int stage_count;
    PicaCombinerStage stages[PICA_COMBINER_MAX_STAGES];
    bool uses_buffer_color;
    uint32_t buffer_color;                    /* 0xRRGGBBAA, for PICA_SRC_PREVIOUS_BUFFER */
    uint8_t unit_texmap[PICA_TEXTURE_UNITS];  /* GX texmap sampled by PICA unit n, or GX_TEV_NONE */
} PicaCombiner;

/* On failure, false with `reason` (if not NULL) set to a static description of
 * the unsupported feature. */
bool gx_tev_translate(const GxTevConfig* gx, PicaCombiner* out, const char** reason);

/* A konst selector's value, 0xRRGGBBAA (alpha selectors: in all channels). */
uint32_t gx_tev_konst_color(const GxTevConfig* gx, uint8_t kcolor_sel);
uint32_t gx_tev_konst_alpha(const GxTevConfig* gx, uint8_t kalpha_sel);

#endif /* AC3DS_GX_TEV_H */
