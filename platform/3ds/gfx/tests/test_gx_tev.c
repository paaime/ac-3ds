#include "gx_tev.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define RGBA(r, g, b, a) \
    (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))
/* One 8-bit step: GX and PICA registers are 8-bit, and constant sub-expressions fold to 8 bits. */
#define TOLERANCE (1.0f / 255.0f + 1e-5f)
#define RANDOM_TRIALS 200
#define TEXMAPS 8

typedef struct {
    float c[4];
} Col;

static Col unpack(uint32_t v)
{
    Col c = { { (v >> 24) / 255.0f, ((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f,
                (v & 0xFF) / 255.0f } };
    return c;
}

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

typedef struct {
    Col tex[TEXMAPS];
    Col ras;
} Inputs;

static float gx_color_arg(const GxTevConfig* g, const GxTevStage* s, const Col* reg, const Inputs* in,
                          int arg, int comp)
{
    const Col* tex = s->tex_map < TEXMAPS ? &in->tex[s->tex_map] : NULL;
    switch (arg) {
    case GXT_CC_CPREV: return reg[0].c[comp];
    case GXT_CC_APREV: return reg[0].c[3];
    case GXT_CC_C0: return reg[1].c[comp];
    case GXT_CC_A0: return reg[1].c[3];
    case GXT_CC_C1: return reg[2].c[comp];
    case GXT_CC_A1: return reg[2].c[3];
    case GXT_CC_C2: return reg[3].c[comp];
    case GXT_CC_A2: return reg[3].c[3];
    case GXT_CC_TEXC: return tex ? tex->c[comp] : 0.0f;
    case GXT_CC_TEXA: return tex ? tex->c[3] : 0.0f;
    case GXT_CC_RASC: return in->ras.c[comp];
    case GXT_CC_RASA: return in->ras.c[3];
    case GXT_CC_ONE: return 1.0f;
    case GXT_CC_HALF: return 128.0f / 255.0f; /* hardware value 0x80, as in Dolphin */
    case GXT_CC_KONST: return unpack(gx_tev_konst_color(g, s->kcolor_sel)).c[comp];
    default: return 0.0f;
    }
}

static float gx_alpha_arg(const GxTevConfig* g, const GxTevStage* s, const Col* reg, const Inputs* in, int arg)
{
    const Col* tex = s->tex_map < TEXMAPS ? &in->tex[s->tex_map] : NULL;
    switch (arg) {
    case GXT_CA_APREV: return reg[0].c[3];
    case GXT_CA_A0: return reg[1].c[3];
    case GXT_CA_A1: return reg[2].c[3];
    case GXT_CA_A2: return reg[3].c[3];
    case GXT_CA_TEXA: return tex ? tex->c[3] : 0.0f;
    case GXT_CA_RASA: return in->ras.c[3];
    case GXT_CA_KONST: return unpack(gx_tev_konst_alpha(g, s->kalpha_sel)).c[3];
    default: return 0.0f;
    }
}

static float gx_combine(const GxTevChannel* ch, float a, float b, float c, float d)
{
    static const float scales[4] = { 1.0f, 2.0f, 4.0f, 0.5f };
    float lerp = a * (1.0f - c) + b * c;
    float v = ch->op == GXT_OP_SUB ? d - lerp : d + lerp;
    v += ch->bias == GXT_BIAS_ADDHALF ? 0.5f : (ch->bias == GXT_BIAS_SUBHALF ? -0.5f : 0.0f);
    v *= scales[ch->scale & 3];
    return ch->clamp ? clamp01(v) : v;
}

static Col gx_evaluate(const GxTevConfig* g, const Inputs* in)
{
    Col reg[4];
    for (int i = 0; i < 4; i++) {
        reg[i] = unpack(g->reg[i]);
    }
    for (int s = 0; s < g->stage_count; s++) {
        const GxTevStage* st = &g->stages[s];
        Col out;
        for (int comp = 0; comp < 3; comp++) {
            float v[4];
            for (int k = 0; k < 4; k++) {
                v[k] = gx_color_arg(g, st, reg, in, st->color.in[k], comp);
            }
            out.c[comp] = gx_combine(&st->color, v[0], v[1], v[2], v[3]);
        }
        float va[4];
        for (int k = 0; k < 4; k++) {
            va[k] = gx_alpha_arg(g, st, reg, in, st->alpha.in[k]);
        }
        out.c[3] = gx_combine(&st->alpha, va[0], va[1], va[2], va[3]);
        for (int comp = 0; comp < 3; comp++) {
            reg[st->color.out_reg].c[comp] = out.c[comp];
        }
        reg[st->alpha.out_reg].c[3] = out.c[3];
    }
    return reg[0];
}

/* The combiner buffer lags a stage: stage 0 reads 0 (Azahar's sw_rasterizer.cpp, from hardware tests). */
static float pica_source(const PicaCombiner* p, int stage_index, const PicaCombinerStage* st, const Inputs* in,
                         Col prev, int src, int comp)
{
    switch (src) {
    case PICA_SRC_PRIMARY: return in->ras.c[comp];
    case PICA_SRC_TEXTURE0:
    case PICA_SRC_TEXTURE1:
    case PICA_SRC_TEXTURE2: {
        uint8_t map = p->unit_texmap[src - PICA_SRC_TEXTURE0];
        return map < TEXMAPS ? in->tex[map].c[comp] : NAN;
    }
    case PICA_SRC_CONSTANT: return unpack(st->constant).c[comp];
    case PICA_SRC_PREVIOUS_BUFFER:
        return p->uses_buffer_color && stage_index > 0 ? unpack(p->buffer_color).c[comp] : NAN;
    case PICA_SRC_PREVIOUS: return prev.c[comp];
    default: return NAN;
    }
}

static float pica_operand(float colour[4], int op, bool alpha_channel, int comp)
{
    if (alpha_channel) {
        return op == PICA_OP_A_ONE_MINUS_ALPHA ? 1.0f - colour[3] : colour[3];
    }
    switch (op) {
    case PICA_OP_RGB_COLOR: return colour[comp];
    case PICA_OP_RGB_ONE_MINUS_COLOR: return 1.0f - colour[comp];
    case PICA_OP_RGB_ALPHA: return colour[3];
    case PICA_OP_RGB_ONE_MINUS_ALPHA: return 1.0f - colour[3];
    default: return NAN;
    }
}

static float pica_func(int func, float s0, float s1, float s2)
{
    switch (func) {
    case PICA_FUNC_REPLACE: return s0;
    case PICA_FUNC_MODULATE: return s0 * s1;
    case PICA_FUNC_ADD: return s0 + s1;
    case PICA_FUNC_ADD_SIGNED: return s0 + s1 - 0.5f;
    case PICA_FUNC_INTERPOLATE: return s0 * s2 + s1 * (1.0f - s2);
    case PICA_FUNC_SUBTRACT: return s0 - s1;
    case PICA_FUNC_MULTIPLY_ADD: return s0 * s1 + s2;
    default: return NAN;
    }
}

static float pica_channel(const PicaCombiner* p, int stage_index, const PicaCombinerOp* op, const Inputs* in,
                          Col prev, bool alpha_channel, int comp)
{
    static const float scales[3] = { 1.0f, 2.0f, 4.0f };
    float s[3];
    for (int k = 0; k < 3; k++) {
        float colour[4];
        for (int c = 0; c < 4; c++) {
            colour[c] = pica_source(p, stage_index, &p->stages[stage_index], in, prev, op->src[k], c);
        }
        s[k] = pica_operand(colour, op->op[k], alpha_channel, comp);
    }
    return clamp01(pica_func(op->func, s[0], s[1], s[2]) * scales[op->scale % 3]);
}

static Col pica_evaluate(const PicaCombiner* p, const Inputs* in)
{
    Col prev = { { NAN, NAN, NAN, NAN } }; /* reading "previous" in stage 0 is a translation bug */
    for (int s = 0; s < p->stage_count; s++) {
        const PicaCombinerStage* st = &p->stages[s];
        Col out;
        for (int comp = 0; comp < 3; comp++) {
            out.c[comp] = pica_channel(p, s, &st->rgb, in, prev, false, comp);
        }
        out.c[3] = pica_channel(p, s, &st->alpha, in, prev, true, 3);
        prev = out;
    }
    return prev;
}

static GxTevChannel color_ch(int a, int b, int c, int d)
{
    GxTevChannel ch = { { (uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d }, GXT_OP_ADD, GXT_BIAS_ZERO,
                        GXT_SCALE_1, true, GXT_REG_PREV };
    return ch;
}

static GxTevStage stage(GxTevChannel color, GxTevChannel alpha, int texmap)
{
    GxTevStage s = { color, alpha, (uint8_t)(texmap == GX_TEV_NONE ? GX_TEV_NONE : 0), (uint8_t)texmap,
                     0x06 /* KCSEL_1_4 */, 0x00 /* KASEL_1 */ };
    return s;
}

static GxTevConfig base_config(void)
{
    GxTevConfig g;
    memset(&g, 0, sizeof(g));
    g.reg[0] = RGBA(0x10, 0x20, 0x30, 0x40);
    g.reg[1] = RGBA(0xFF, 0x00, 0x80, 0x55);
    g.reg[2] = RGBA(0xFF, 0xB0, 0x60, 0xC0);
    g.reg[3] = RGBA(0x30, 0x60, 0xF0, 0x90);
    g.konst[0] = RGBA(0x12, 0x34, 0x56, 0x78);
    return g;
}

static Inputs random_inputs(void)
{
    Inputs in;
    for (int t = 0; t < TEXMAPS; t++) {
        for (int c = 0; c < 4; c++) {
            in.tex[t].c[c] = (float)(rand() % 256) / 255.0f;
        }
    }
    for (int c = 0; c < 4; c++) {
        in.ras.c[c] = (float)(rand() % 256) / 255.0f;
    }
    return in;
}

static bool matches_reference(const char* name, const GxTevConfig* g, PicaCombiner* out)
{
    const char* reason = NULL;
    if (!gx_tev_translate(g, out, &reason)) {
        fprintf(stderr, "%s: translation failed: %s\n", name, reason ? reason : "?");
        return false;
    }
    for (int trial = 0; trial < RANDOM_TRIALS; trial++) {
        Inputs in = random_inputs();
        Col want = gx_evaluate(g, &in);
        Col got = pica_evaluate(out, &in);
        for (int c = 0; c < 4; c++) {
            if (!(fabsf(want.c[c] - got.c[c]) < TOLERANCE)) {
                fprintf(stderr, "%s: mismatch channel %d: gx %f pica %f\n", name, c, want.c[c], got.c[c]);
                return false;
            }
        }
    }
    return true;
}

static void test_texture_times_shade(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 1;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    PicaCombiner p;
    CHECK(matches_reference("tex*shade", &g, &p));
    CHECK(p.stage_count == 1);
    CHECK(p.stages[0].rgb.func == PICA_FUNC_MODULATE);
    CHECK(p.stages[0].alpha.func == PICA_FUNC_REPLACE);
    CHECK(p.unit_texmap[0] == 0);
}

static void test_ac_dominant_combiner(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 2;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    g.stages[1] = stage(color_ch(GXT_CC_ZERO, GXT_CC_C1, GXT_CC_CPREV, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("ac dominant", &g, &p));
    CHECK(p.stage_count == 2);
    CHECK(!p.uses_buffer_color);
}

static void test_lerp_with_two_constants_uses_buffer(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 1;
    g.stages[0] = stage(color_ch(GXT_CC_C2, GXT_CC_C1, GXT_CC_TEXC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_TEXA, GXT_CA_A1, GXT_CA_ZERO), 0);
    PicaCombiner p;
    CHECK(matches_reference("lerp env prim", &g, &p));
    CHECK(p.uses_buffer_color);
    CHECK(p.stage_count == 2);
    CHECK(p.stages[0].rgb.func == PICA_FUNC_REPLACE && p.stages[0].rgb.src[0] == PICA_SRC_PRIMARY);
    CHECK(p.stages[1].rgb.func == PICA_FUNC_INTERPOLATE);
}

static void test_buffer_already_past_stage0_needs_no_shift(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 2;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    g.stages[1] = stage(color_ch(GXT_CC_C2, GXT_CC_C1, GXT_CC_CPREV, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("buffer in stage 1", &g, &p));
    CHECK(p.uses_buffer_color);
    CHECK(p.stage_count == 2);
}

static void test_buffer_in_stage0_with_no_room_fails(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 6;
    g.stages[0] = stage(color_ch(GXT_CC_C2, GXT_CC_C1, GXT_CC_TEXC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    for (int i = 1; i < 6; i++) {
        g.stages[i] = stage(color_ch(GXT_CC_ZERO, GXT_CC_CPREV, GXT_CC_RASC, GXT_CC_ZERO),
                            color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    }
    PicaCombiner p;
    const char* reason = NULL;
    CHECK(!gx_tev_translate(&g, &p, &reason));
    CHECK(reason != NULL);
}

static void test_previous_plus_general_lerp(void)
{
    static const uint8_t SHAPES[3][2] = { { GXT_CC_C2, GXT_CC_C1 }, { GXT_CC_CPREV, GXT_CC_C1 },
                                          { GXT_CC_C2, GXT_CC_CPREV } };
    for (int i = 0; i < 3; i++) {
        GxTevConfig g = base_config();
        g.stage_count = 2;
        g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_ZERO),
                            color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
        GxTevChannel ch = color_ch(SHAPES[i][0], SHAPES[i][1], GXT_CC_TEXC, GXT_CC_CPREV);
        ch.scale = GXT_SCALE_2;
        g.stages[1] = stage(ch, color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), 0);
        PicaCombiner p;
        CHECK(matches_reference("prev + lerp", &g, &p));
        CHECK(p.stage_count == 3);
        CHECK(p.stages[1].rgb.func == PICA_FUNC_MULTIPLY_ADD && p.stages[2].rgb.func == PICA_FUNC_MULTIPLY_ADD);
    }
}

static void test_multiply_add_two_constants(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 1;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_C1, GXT_CC_RASC, GXT_CC_C2),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_RASA), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("prim*shade+env", &g, &p));
    CHECK(p.stage_count == 2);
}

static void test_constant_and_previous_register(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 2;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_CPREV),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    g.stages[1] = stage(color_ch(GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_C1),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_KONST), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("prev register + const", &g, &p));
}

static void test_subtract_scale_and_interpolate_plus_d(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 3;
    GxTevChannel sub = color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_C1);
    sub.op = GXT_OP_SUB;
    g.stages[0] = stage(sub, color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    GxTevChannel scaled = color_ch(GXT_CC_ZERO, GXT_CC_CPREV, GXT_CC_RASC, GXT_CC_ZERO);
    scaled.scale = GXT_SCALE_2;
    g.stages[1] = stage(scaled, color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    g.stages[2] = stage(color_ch(GXT_CC_C2, GXT_CC_CPREV, GXT_CC_TEXA, GXT_CC_RASC),
                        color_ch(GXT_CA_ZERO, GXT_CA_APREV, GXT_CA_RASA, GXT_CA_ZERO), 0);
    PicaCombiner p;
    CHECK(matches_reference("sub/scale/lerp+d", &g, &p));
    CHECK(p.stage_count <= PICA_COMBINER_MAX_STAGES);
}

static void test_one_minus_and_trivial_lerps(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 3;
    g.stages[0] = stage(color_ch(GXT_CC_TEXC, GXT_CC_ZERO, GXT_CC_RASA, GXT_CC_ZERO),
                        color_ch(GXT_CA_TEXA, GXT_CA_ZERO, GXT_CA_RASA, GXT_CA_ZERO), 0);
    g.stages[1] = stage(color_ch(GXT_CC_RASC, GXT_CC_CPREV, GXT_CC_ONE, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    g.stages[2] = stage(color_ch(GXT_CC_CPREV, GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_HALF),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("one-minus / trivial", &g, &p));
}

static void test_sub_half_bias_becomes_add_signed(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 1;
    GxTevChannel ch = color_ch(GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_ZERO, GXT_CC_RASC);
    ch.in[0] = GXT_CC_TEXC;
    ch.bias = GXT_BIAS_SUBHALF;
    g.stages[0] = stage(ch, color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);
    PicaCombiner p;
    /* The GX model uses exactly -0.5 while PICA's ADD_SIGNED uses -128/255. */
    const char* reason = NULL;
    CHECK(gx_tev_translate(&g, &p, &reason));
    CHECK(p.stages[0].rgb.func == PICA_FUNC_ADD_SIGNED);
}

static void test_more_konst_selectors(void)
{
    GxTevConfig g = base_config();
    g.konst[1] = RGBA(0x01, 0x02, 0x03, 0x04);
    CHECK(gx_tev_konst_color(&g, 0x15) == RGBA(0x02, 0x02, 0x02, 0x02)); /* K1_G */
    CHECK(gx_tev_konst_color(&g, 0x19) == RGBA(0x03, 0x03, 0x03, 0x03)); /* K1_B */
    CHECK(gx_tev_konst_color(&g, 0x08) == RGBA(255, 255, 255, 255));     /* reserved -> 1 */
    CHECK(gx_tev_konst_alpha(&g, 0x0C) == RGBA(255, 255, 255, 255));     /* no K0 for alpha */
}

static void test_three_constants_split_across_stages(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 2;
    g.stages[0] = stage(color_ch(GXT_CC_C0, GXT_CC_C1, GXT_CC_C2, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_A1), GX_TEV_NONE);
    g.stages[1] = stage(color_ch(GXT_CC_ZERO, GXT_CC_C1, GXT_CC_A0, GXT_CC_C2),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), GX_TEV_NONE);
    PicaCombiner p;
    CHECK(matches_reference("three constants", &g, &p));
    CHECK(p.stage_count == 2);
}

static void test_konst_selectors(void)
{
    GxTevConfig g = base_config();
    CHECK(gx_tev_konst_color(&g, 0x00) == RGBA(255, 255, 255, 255)); /* KCSEL_1 */
    CHECK(gx_tev_konst_color(&g, 0x06) == RGBA(63, 63, 63, 63));     /* KCSEL_1_4 */
    CHECK(gx_tev_konst_color(&g, 0x0C) == g.konst[0]);               /* K0 */
    CHECK(gx_tev_konst_color(&g, 0x10) == RGBA(0x12, 0x12, 0x12, 0x12)); /* K0_R */
    CHECK(gx_tev_konst_alpha(&g, 0x1C) == RGBA(0x78, 0x78, 0x78, 0x78)); /* K0_A */
    CHECK(gx_tev_konst_alpha(&g, 0x04) == RGBA(127, 127, 127, 127));     /* KASEL_1_2 */
}

#include "pc_shader_seed.h" /* pc/include: pc_shader_seed_keys[139][122] */

#define SEED_HEADER_BYTES 29 /* num_stages, num_ind, fog, alpha[3], light[7], swap_tbl[16] */
#define SEED_STAGE_BYTES  31

static uint32_t random_rgba(void)
{
    return ((uint32_t)rand() << 16) ^ (uint32_t)rand();
}

/* PCGXShaderKey (pc_gx_internal.h) has no texmap ids: each texturing stage samples texmap == texcoord. */
static GxTevConfig config_from_seed(const unsigned char* key)
{
    GxTevConfig g;
    memset(&g, 0, sizeof(g));
    g.stage_count = key[0];
    for (int i = 0; i < 4; i++) {
        g.reg[i] = random_rgba();
        g.konst[i] = random_rgba();
    }
    for (int s = 0; s < g.stage_count; s++) {
        const unsigned char* st = &key[SEED_HEADER_BYTES + s * SEED_STAGE_BYTES];
        GxTevStage* t = &g.stages[s];
        memcpy(t->color.in, &st[0], 4);
        memcpy(t->alpha.in, &st[4], 4);
        t->color.op = st[8];
        t->alpha.op = st[9];
        t->color.bias = st[10];
        t->color.scale = st[11];
        t->alpha.bias = st[12];
        t->alpha.scale = st[13];
        t->color.clamp = st[14] != 0;
        t->alpha.clamp = st[15] != 0;
        t->color.out_reg = st[16];
        t->alpha.out_reg = st[17];
        t->kcolor_sel = st[20];
        t->kalpha_sel = st[21];
        bool textured = st[23] != 0;
        t->tex_coord = textured ? st[22] : GX_TEV_NONE;
        t->tex_map = textured ? st[22] : GX_TEV_NONE;
    }
    return g;
}

static void test_recorded_gameplay_configs(void)
{
    int translated = 0;
    int stage_histogram[PICA_COMBINER_MAX_STAGES + 1] = { 0 };
    for (int i = 0; i < PC_SHADER_SEED_COUNT; i++) {
        GxTevConfig g = config_from_seed(pc_shader_seed_keys[i]);
        PicaCombiner p;
        char name[32];
        snprintf(name, sizeof(name), "seed %d", i);
        if (matches_reference(name, &g, &p)) {
            translated++;
            stage_histogram[p.stage_count]++;
        }
    }
    printf("gx_tev: %d/%d recorded gameplay configs translate and match (PICA stages:", translated,
           PC_SHADER_SEED_COUNT);
    for (int n = 1; n <= PICA_COMBINER_MAX_STAGES; n++) {
        if (stage_histogram[n] != 0) {
            printf(" %d=%d", n, stage_histogram[n]);
        }
    }
    printf(")\n");
    CHECK(translated == PC_SHADER_SEED_COUNT);
}

static void expect_unsupported(const char* name, const GxTevConfig* g)
{
    PicaCombiner p;
    const char* reason = NULL;
    bool ok = gx_tev_translate(g, &p, &reason);
    if (ok || reason == NULL) {
        fprintf(stderr, "%s: expected an unsupported-feature failure\n", name);
    }
    CHECK(!ok);
    CHECK(reason != NULL);
}

static void test_unsupported_features(void)
{
    GxTevConfig g = base_config();
    g.stage_count = 1;
    g.stages[0] = stage(color_ch(GXT_CC_ZERO, GXT_CC_TEXC, GXT_CC_RASC, GXT_CC_ZERO),
                        color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_TEXA), 0);

    GxTevConfig reg_out = g;
    reg_out.stages[0].color.out_reg = GXT_REG_0;
    expect_unsupported("out reg", &reg_out);

    GxTevConfig add_half = g;
    add_half.stages[0].color.bias = GXT_BIAS_ADDHALF;
    expect_unsupported("add half", &add_half);

    GxTevConfig half_scale = g;
    half_scale.stages[0].color.scale = GXT_SCALE_HALF;
    expect_unsupported("scale 1/2", &half_scale);

    GxTevConfig too_many = g;
    too_many.stage_count = 7;
    for (int i = 0; i < 7; i++) {
        too_many.stages[i] = g.stages[0];
    }
    expect_unsupported("seven stages", &too_many);

    GxTevConfig texcoord_clash = g;
    texcoord_clash.stage_count = 2;
    texcoord_clash.stages[1] = g.stages[0];
    texcoord_clash.stages[1].tex_map = 1;
    expect_unsupported("texcoord clash", &texcoord_clash);

    GxTevConfig prev_minus = g;
    prev_minus.stage_count = 2;
    GxTevChannel sub = color_ch(GXT_CC_C2, GXT_CC_C1, GXT_CC_TEXC, GXT_CC_CPREV);
    sub.op = GXT_OP_SUB;
    prev_minus.stages[1] = stage(sub, color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), 0);
    expect_unsupported("PREV - two-op lerp", &prev_minus);

    GxTevConfig prev_everywhere = g;
    prev_everywhere.stage_count = 2;
    prev_everywhere.stages[1] = stage(color_ch(GXT_CC_C2, GXT_CC_C1, GXT_CC_APREV, GXT_CC_CPREV),
                                      color_ch(GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_ZERO, GXT_CA_APREV), 0);
    expect_unsupported("PREV + lerp(.., .., PREV)", &prev_everywhere);

    GxTevConfig prev_ab = prev_everywhere;
    prev_ab.stages[1].color.in[0] = GXT_CC_CPREV;
    prev_ab.stages[1].color.in[1] = GXT_CC_APREV;
    prev_ab.stages[1].color.in[2] = GXT_CC_TEXC;
    expect_unsupported("PREV + lerp(PREV, PREV, ..)", &prev_ab);

    GxTevConfig coord2 = g;
    coord2.stages[0].tex_coord = GX_TEV_MAX_TEXCOORDS;
    expect_unsupported("texcoord 2", &coord2);

    GxTevConfig no_map = g;
    no_map.stages[0].tex_map = GX_TEV_NONE;
    expect_unsupported("texture without texmap", &no_map);

    GxTevConfig none = g;
    none.stage_count = 0;
    expect_unsupported("no stages", &none);
}

int main(void)
{
    srand(12345);
    test_texture_times_shade();
    test_ac_dominant_combiner();
    test_lerp_with_two_constants_uses_buffer();
    test_buffer_already_past_stage0_needs_no_shift();
    test_buffer_in_stage0_with_no_room_fails();
    test_previous_plus_general_lerp();
    test_multiply_add_two_constants();
    test_constant_and_previous_register();
    test_subtract_scale_and_interpolate_plus_d();
    test_one_minus_and_trivial_lerps();
    test_konst_selectors();
    test_more_konst_selectors();
    test_sub_half_bias_becomes_add_signed();
    test_three_constants_split_across_stages();
    test_recorded_gameplay_configs();
    test_unsupported_features();

    if (failures != 0) {
        fprintf(stderr, "gx_tev: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gx_tev: all tests passed\n");
    return 0;
}
