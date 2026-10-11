#include "gx_tev.h"

#include <string.h>

/* ---- terms: a resolved GX input ---------------------------------------------- */

typedef enum {
    TERM_ZERO,
    TERM_SRC,   /* a PICA source (primary colour, texture, previous stage) */
    TERM_CONST, /* a constant colour: needs the stage constant or the buffer colour */
} TermKind;

typedef struct {
    TermKind kind;
    uint8_t src;     /* TERM_SRC: PICA_SRC_* */
    bool read_alpha; /* TERM_SRC in the colour channel: replicate the source's alpha */
    uint32_t value;  /* TERM_CONST: 0xRRGGBBAA (colour channel: rgb; alpha channel: a) */
} Term;

typedef struct {
    uint8_t func;
    Term t[3];
    bool one_minus[3];
} AbstractOp;

#define MAX_OPS_PER_CHANNEL 2

typedef struct {
    AbstractOp ops[MAX_OPS_PER_CHANNEL];
    int count;
    uint8_t scale; /* PICA_SCALE_*, applied to the last op */
} ChannelProgram;

/* Constant slots of one PICA stage (or of the global buffer colour). */
typedef struct {
    bool rgb_set;
    bool alpha_set;
    uint32_t value;
} ConstSlot;

typedef struct {
    const GxTevConfig* gx;
    PicaCombiner* out;
    ConstSlot buffer;
    const char* reason;
} Translator;

#define RGB_MASK   0xFFFFFF00u
#define ALPHA_MASK 0x000000FFu

static Term term_zero(void)
{
    Term t = { TERM_ZERO, 0, false, 0 };
    return t;
}

static Term term_src(uint8_t src, bool read_alpha)
{
    Term t = { TERM_SRC, src, read_alpha, 0 };
    return t;
}

static Term term_const(uint32_t value)
{
    Term t = { TERM_CONST, 0, false, value };
    return t;
}

static uint32_t replicate_alpha(uint32_t rgba)
{
    uint32_t a = rgba & 0xFF;
    return (a << 24) | (a << 16) | (a << 8) | a;
}

static uint32_t fraction(uint32_t eighths)
{
    uint32_t v = 255u * eighths / 8u; /* 255, 223, 191, ... 31, as Dolphin */
    return (v << 24) | (v << 16) | (v << 8) | v;
}

/* ---- konst selectors --------------------------------------------------------- */

#define KSEL_FRACTION_LAST 0x07
#define KSEL_K0            0x0C
#define KSEL_K0_R          0x10
#define KSEL_K0_G          0x14
#define KSEL_K0_B          0x18
#define KSEL_K0_A          0x1C
#define KSEL_LAST          0x1F

static uint32_t konst_channel(const GxTevConfig* gx, uint8_t sel, uint8_t base, int shift)
{
    uint32_t v = (gx->konst[(sel - base) & 3] >> shift) & 0xFF;
    return (v << 24) | (v << 16) | (v << 8) | v;
}

uint32_t gx_tev_konst_color(const GxTevConfig* gx, uint8_t sel)
{
    if (sel <= KSEL_FRACTION_LAST) {
        return fraction(8u - sel);
    }
    if (sel >= KSEL_K0 && sel < KSEL_K0_R) {
        return gx->konst[sel - KSEL_K0];
    }
    if (sel >= KSEL_K0_A && sel <= KSEL_LAST) {
        return konst_channel(gx, sel, KSEL_K0_A, 0);
    }
    if (sel >= KSEL_K0_B) {
        return konst_channel(gx, sel, KSEL_K0_B, 8);
    }
    if (sel >= KSEL_K0_G) {
        return konst_channel(gx, sel, KSEL_K0_G, 16);
    }
    if (sel >= KSEL_K0_R) {
        return konst_channel(gx, sel, KSEL_K0_R, 24);
    }
    return fraction(8u); /* reserved values: 1.0 */
}

uint32_t gx_tev_konst_alpha(const GxTevConfig* gx, uint8_t sel)
{
    /* Alpha selectors share the colour numbering, minus the full-colour K0..K3. */
    if (sel >= KSEL_K0 && sel < KSEL_K0_R) {
        return fraction(8u);
    }
    return replicate_alpha(gx_tev_konst_color(gx, sel));
}

/* ---- input resolution --------------------------------------------------------- */

static bool assign_texture_unit(Translator* tr, const GxTevStage* s, Term* t, bool read_alpha)
{
    if (s->tex_map == GX_TEV_NONE || s->tex_coord >= GX_TEV_MAX_TEXCOORDS) {
        tr->reason = s->tex_map == GX_TEV_NONE ? "texture input without a texmap"
                                               : "texcoord > 1 (the shader generates two)";
        return false;
    }
    uint8_t unit = s->tex_coord;
    if (tr->out->unit_texmap[unit] == GX_TEV_NONE) {
        tr->out->unit_texmap[unit] = s->tex_map;
    } else if (tr->out->unit_texmap[unit] != s->tex_map) {
        tr->reason = "one texcoord used with two texmaps";
        return false;
    }
    *t = term_src((uint8_t)(PICA_SRC_TEXTURE0 + unit), read_alpha);
    return true;
}

static Term prev_term(const Translator* tr, bool has_previous, bool read_alpha)
{
    if (has_previous) {
        return term_src(PICA_SRC_PREVIOUS, read_alpha);
    }
    /* No earlier PICA stage: PREV still holds its initial register value. */
    uint32_t reg = tr->gx->reg[GXT_REG_PREV];
    return term_const(read_alpha ? replicate_alpha(reg) : reg);
}

static bool color_term(Translator* tr, const GxTevStage* s, uint8_t arg, bool has_previous, Term* t)
{
    const uint32_t* reg = tr->gx->reg;
    switch (arg) {
    case GXT_CC_CPREV: *t = prev_term(tr, has_previous, false); return true;
    case GXT_CC_APREV: *t = prev_term(tr, has_previous, true); return true;
    case GXT_CC_C0: *t = term_const(reg[1]); return true;
    case GXT_CC_A0: *t = term_const(replicate_alpha(reg[1])); return true;
    case GXT_CC_C1: *t = term_const(reg[2]); return true;
    case GXT_CC_A1: *t = term_const(replicate_alpha(reg[2])); return true;
    case GXT_CC_C2: *t = term_const(reg[3]); return true;
    case GXT_CC_A2: *t = term_const(replicate_alpha(reg[3])); return true;
    case GXT_CC_TEXC: return assign_texture_unit(tr, s, t, false);
    case GXT_CC_TEXA: return assign_texture_unit(tr, s, t, true);
    case GXT_CC_RASC: *t = term_src(PICA_SRC_PRIMARY, false); return true;
    case GXT_CC_RASA: *t = term_src(PICA_SRC_PRIMARY, true); return true;
    case GXT_CC_ONE: *t = term_const(0xFFFFFFFFu); return true;
    case GXT_CC_HALF: *t = term_const(0x80808080u); return true;
    case GXT_CC_KONST: *t = term_const(gx_tev_konst_color(tr->gx, s->kcolor_sel)); return true;
    case GXT_CC_ZERO: *t = term_zero(); return true;
    default: tr->reason = "unknown colour input"; return false;
    }
}

static bool alpha_term(Translator* tr, const GxTevStage* s, uint8_t arg, bool has_previous, Term* t)
{
    const uint32_t* reg = tr->gx->reg;
    switch (arg) {
    case GXT_CA_APREV: *t = prev_term(tr, has_previous, true); return true;
    case GXT_CA_A0: *t = term_const(reg[1]); return true;
    case GXT_CA_A1: *t = term_const(reg[2]); return true;
    case GXT_CA_A2: *t = term_const(reg[3]); return true;
    case GXT_CA_TEXA: return assign_texture_unit(tr, s, t, true);
    case GXT_CA_RASA: *t = term_src(PICA_SRC_PRIMARY, true); return true;
    case GXT_CA_KONST: *t = term_const(gx_tev_konst_alpha(tr->gx, s->kalpha_sel)); return true;
    case GXT_CA_ZERO: *t = term_zero(); return true;
    default: tr->reason = "unknown alpha input"; return false;
    }
}

/* ---- term algebra -------------------------------------------------------------- */

static uint32_t relevant(uint32_t value, bool alpha_channel)
{
    return value & (alpha_channel ? ALPHA_MASK : RGB_MASK);
}

static bool is_zero(const Term* t, bool alpha_channel)
{
    return t->kind == TERM_ZERO || (t->kind == TERM_CONST && relevant(t->value, alpha_channel) == 0);
}

static bool is_one(const Term* t, bool alpha_channel)
{
    return t->kind == TERM_CONST &&
           relevant(t->value, alpha_channel) == relevant(0xFFFFFFFFu, alpha_channel);
}

static bool same_term(const Term* a, const Term* b, bool alpha_channel)
{
    if (is_zero(a, alpha_channel) || is_zero(b, alpha_channel)) {
        return is_zero(a, alpha_channel) && is_zero(b, alpha_channel);
    }
    if (a->kind != b->kind) {
        return false;
    }
    if (a->kind == TERM_CONST) {
        return relevant(a->value, alpha_channel) == relevant(b->value, alpha_channel);
    }
    return a->src == b->src && (alpha_channel || a->read_alpha == b->read_alpha);
}

static bool reads_previous(const Term* t)
{
    return t->kind == TERM_SRC && t->src == PICA_SRC_PREVIOUS;
}

static AbstractOp op1(uint8_t func, Term a)
{
    AbstractOp op = { func, { a, term_zero(), term_zero() }, { false, false, false } };
    return op;
}

static AbstractOp op2(uint8_t func, Term a, Term b)
{
    AbstractOp op = { func, { a, b, term_zero() }, { false, false, false } };
    return op;
}

static AbstractOp op3(uint8_t func, Term a, Term b, Term c)
{
    AbstractOp op = { func, { a, b, c }, { false, false, false } };
    return op;
}

typedef enum {
    LERP_ZERO,    /* 0 */
    LERP_SINGLE,  /* x */
    LERP_PRODUCT, /* x * y (y possibly one-minus) */
    LERP_GENERAL, /* a*(1-c) + b*c */
} LerpShape;

typedef struct {
    LerpShape shape;
    AbstractOp op; /* how to compute the lerp in one PICA op */
} Lerp;

static Lerp classify_lerp(const Term in[4], bool alpha_channel)
{
    const Term* a = &in[0];
    const Term* b = &in[1];
    const Term* c = &in[2];
    Lerp l;
    if (is_zero(c, alpha_channel) || same_term(a, b, alpha_channel)) {
        l.shape = is_zero(a, alpha_channel) ? LERP_ZERO : LERP_SINGLE;
        l.op = op1(PICA_FUNC_REPLACE, *a);
    } else if (is_one(c, alpha_channel)) {
        l.shape = is_zero(b, alpha_channel) ? LERP_ZERO : LERP_SINGLE;
        l.op = op1(PICA_FUNC_REPLACE, *b);
    } else if (is_zero(a, alpha_channel) && is_zero(b, alpha_channel)) {
        l.shape = LERP_ZERO;
        l.op = op1(PICA_FUNC_REPLACE, term_zero());
    } else if (is_zero(a, alpha_channel)) {
        l.shape = LERP_PRODUCT;
        l.op = op2(PICA_FUNC_MODULATE, *b, *c);
    } else if (is_zero(b, alpha_channel)) {
        l.shape = LERP_PRODUCT;
        l.op = op2(PICA_FUNC_MODULATE, *a, *c);
        l.op.one_minus[1] = true;
    } else {
        l.shape = LERP_GENERAL;
        l.op = op3(PICA_FUNC_INTERPOLATE, *b, *a, *c); /* src0*src2 + src1*(1-src2) */
    }
    return l;
}

static int operand_count(uint8_t func)
{
    if (func == PICA_FUNC_REPLACE) {
        return 1;
    }
    return func == PICA_FUNC_INTERPOLATE || func == PICA_FUNC_MULTIPLY_ADD ? 3 : 2;
}

static bool is_constant(const Term* t)
{
    return t->kind == TERM_CONST || t->kind == TERM_ZERO; /* zero is emitted as constant 0 */
}

/* ---- constant folding: all-constant ops are computed once, on the CPU ---- */

static float component(const Term* t, int shift)
{
    return t->kind == TERM_ZERO ? 0.0f : (float)((t->value >> shift) & 0xFF) / 255.0f;
}

static float fold_value(const AbstractOp* op, int shift)
{
    float v[3];
    for (int k = 0; k < 3; k++) {
        float x = component(&op->t[k], shift);
        v[k] = op->one_minus[k] ? 1.0f - x : x;
    }
    float r;
    switch (op->func) {
    case PICA_FUNC_MODULATE: r = v[0] * v[1]; break;
    case PICA_FUNC_ADD: r = v[0] + v[1]; break;
    case PICA_FUNC_ADD_SIGNED: r = v[0] + v[1] - 0.5f; break;
    case PICA_FUNC_INTERPOLATE: r = v[0] * v[2] + v[1] * (1.0f - v[2]); break;
    case PICA_FUNC_SUBTRACT: r = v[0] - v[1]; break;
    case PICA_FUNC_MULTIPLY_ADD: r = v[0] * v[1] + v[2]; break;
    default: r = v[0]; break;
    }
    return r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r); /* the PICA clamps every stage */
}

static bool all_constant(const AbstractOp* op)
{
    for (int k = 0; k < operand_count(op->func); k++) {
        if (!is_constant(&op->t[k])) {
            return false;
        }
    }
    return true;
}

/* Value of an all-constant op, as an 8-bit constant (the hardware precision). */
static Term folded(const AbstractOp* op, bool alpha_channel)
{
    uint32_t value = 0;
    for (int shift = alpha_channel ? 0 : 8; shift <= (alpha_channel ? 0 : 24); shift += 8) {
        uint32_t c = (uint32_t)(fold_value(op, shift) * 255.0f + 0.5f);
        value |= c << shift;
    }
    return term_const(value);
}

/* Folds leading all-constant ops into the next op's PREVIOUS operands. This
 * removes any op with three distinct constants (a stage reaches only two).
 * keep_last: leave the final ADD that bias -1/2 turns into ADD_SIGNED. */
static void fold_constants(ChannelProgram* prog, bool alpha_channel, bool keep_last)
{
    while (all_constant(&prog->ops[0]) && prog->ops[0].func != PICA_FUNC_REPLACE &&
           !(keep_last && prog->count == 1)) {
        Term value = folded(&prog->ops[0], alpha_channel);
        if (prog->count == 1) {
            prog->ops[0] = op1(PICA_FUNC_REPLACE, value);
            return;
        }
        AbstractOp next = prog->ops[1];
        for (int k = 0; k < 3; k++) {
            if (reads_previous(&next.t[k])) {
                next.t[k] = value;
            }
        }
        for (int i = 1; i < prog->count; i++) {
            prog->ops[i - 1] = prog->ops[i];
        }
        prog->ops[0] = next;
        prog->count--;
    }
}

/* d + lerp(a, b, c) when d is PREVIOUS ("lerp, then add d" would overwrite d):
 * (d + a*(1-c)) + b*c as two MULTIPLY_ADDs (or a and b swapped), exact under
 * the per-stage clamp as both terms are >= 0. The second op must not read PREVIOUS. */
static bool lerp_onto_previous(const Term in[4], ChannelProgram* prog)
{
    const Term* a = &in[0];
    const Term* b = &in[1];
    const Term* c = &in[2];
    if (reads_previous(c) || (reads_previous(a) && reads_previous(b))) {
        return false;
    }
    bool a_first = !reads_previous(b);
    prog->count = 2;
    prog->ops[0] = op3(PICA_FUNC_MULTIPLY_ADD, a_first ? *a : *b, *c, in[3]);
    prog->ops[0].one_minus[1] = a_first;
    prog->ops[1] = op3(PICA_FUNC_MULTIPLY_ADD, a_first ? *b : *a, *c, term_src(PICA_SRC_PREVIOUS, false));
    prog->ops[1].one_minus[1] = !a_first;
    return true;
}

static bool build_channel(Translator* tr, const GxTevChannel* ch, const Term in[4], bool alpha_channel,
                          ChannelProgram* prog)
{
    static const uint8_t SCALES[3] = { PICA_SCALE_1, PICA_SCALE_2, PICA_SCALE_4 };
    if (ch->out_reg != GXT_REG_PREV) {
        tr->reason = "TEV output to REG0-2";
        return false;
    }
    if (ch->scale == GXT_SCALE_HALF || ch->scale > GXT_SCALE_HALF) {
        tr->reason = "TEV scale 1/2";
        return false;
    }
    if (ch->bias == GXT_BIAS_ADDHALF || ch->bias > GXT_BIAS_SUBHALF) {
        tr->reason = "TEV bias +1/2";
        return false;
    }
    if (ch->op != GXT_OP_ADD && ch->op != GXT_OP_SUB) {
        tr->reason = "TEV compare ops";
        return false;
    }

    const Term* d = &in[3];
    bool d_zero = is_zero(d, alpha_channel);
    Lerp l = classify_lerp(in, alpha_channel);
    prog->count = 1;
    prog->scale = SCALES[ch->scale];

    if (l.shape == LERP_ZERO) {
        prog->ops[0] = op1(PICA_FUNC_REPLACE, d_zero ? term_zero() : *d); /* d +/- 0 */
    } else if (ch->op == GXT_OP_ADD && d_zero) {
        prog->ops[0] = l.op;
    } else if (ch->op == GXT_OP_ADD && l.shape == LERP_SINGLE) {
        prog->ops[0] = op2(PICA_FUNC_ADD, l.op.t[0], *d);
    } else if (ch->op == GXT_OP_ADD && l.shape == LERP_PRODUCT) {
        prog->ops[0] = op3(PICA_FUNC_MULTIPLY_ADD, l.op.t[0], l.op.t[1], *d);
        prog->ops[0].one_minus[1] = l.op.one_minus[1];
    } else if (ch->op == GXT_OP_SUB && d_zero) {
        prog->ops[0] = op1(PICA_FUNC_REPLACE, term_zero()); /* 0 - x clamps to 0 */
    } else if (ch->op == GXT_OP_SUB && l.shape == LERP_SINGLE) {
        prog->ops[0] = op2(PICA_FUNC_SUBTRACT, *d, l.op.t[0]);
    } else if (reads_previous(d)) {
        if (ch->op == GXT_OP_SUB) {
            tr->reason = "PREV minus a two-op lerp";
            return false;
        }
        if (!lerp_onto_previous(in, prog)) {
            tr->reason = "PREV + lerp with PREV in b/c";
            return false;
        }
    } else {
        /* Two ops: compute the lerp, then combine it (now PREVIOUS) with d. */
        prog->count = 2;
        prog->ops[0] = l.op;
        prog->ops[1] = ch->op == GXT_OP_SUB ? op2(PICA_FUNC_SUBTRACT, *d, term_src(PICA_SRC_PREVIOUS, false))
                                            : op2(PICA_FUNC_ADD, term_src(PICA_SRC_PREVIOUS, false), *d);
    }

    fold_constants(prog, alpha_channel, ch->bias == GXT_BIAS_SUBHALF);
    if (ch->bias == GXT_BIAS_SUBHALF) {
        AbstractOp* last = &prog->ops[prog->count - 1];
        if (last->func != PICA_FUNC_ADD) {
            tr->reason = "TEV bias -1/2 outside an add";
            return false;
        }
        last->func = PICA_FUNC_ADD_SIGNED;
    }
    return true;
}

/* ---- constant allocation and PICA op emission ------------------------------------ */

static bool slot_take(ConstSlot* slot, uint32_t value, bool alpha_channel)
{
    bool* set = alpha_channel ? &slot->alpha_set : &slot->rgb_set;
    uint32_t mask = alpha_channel ? ALPHA_MASK : RGB_MASK;
    if (!*set) {
        slot->value = (slot->value & ~mask) | (value & mask);
        *set = true;
        return true;
    }
    return (slot->value & mask) == (value & mask);
}

static bool emit_operand(Translator* tr, ConstSlot* stage_const, const Term* t, bool one_minus,
                         bool alpha_channel, uint8_t* src, uint8_t* op)
{
    uint8_t direct = alpha_channel ? PICA_OP_A_ALPHA : PICA_OP_RGB_COLOR;
    uint8_t inverted = alpha_channel ? PICA_OP_A_ONE_MINUS_ALPHA : PICA_OP_RGB_ONE_MINUS_COLOR;
    if (t->kind == TERM_SRC) {
        *src = t->src;
        if (!alpha_channel && t->read_alpha) {
            *op = one_minus ? PICA_OP_RGB_ONE_MINUS_ALPHA : PICA_OP_RGB_ALPHA;
        } else {
            *op = one_minus ? inverted : direct;
        }
        return true;
    }
    uint32_t value = t->kind == TERM_ZERO ? 0 : t->value;
    *op = one_minus ? inverted : direct;
    if (slot_take(stage_const, value, alpha_channel)) {
        *src = PICA_SRC_CONSTANT;
        return true;
    }
    if (slot_take(&tr->buffer, value, alpha_channel)) {
        *src = PICA_SRC_PREVIOUS_BUFFER;
        return true;
    }
    tr->reason = "more than two distinct constants";
    return false;
}

static bool emit_op(Translator* tr, ConstSlot* stage_const, const AbstractOp* a, bool alpha_channel,
                    uint8_t scale, PicaCombinerOp* out)
{
    out->func = a->func;
    out->scale = scale;
    int used = a->func == PICA_FUNC_REPLACE ? 1 : (a->func == PICA_FUNC_INTERPOLATE ||
                                                   a->func == PICA_FUNC_MULTIPLY_ADD ? 3 : 2);
    for (int k = 0; k < 3; k++) {
        if (k >= used) {
            out->src[k] = PICA_SRC_PREVIOUS; /* unused operand: any source */
            out->op[k] = alpha_channel ? PICA_OP_A_ALPHA : PICA_OP_RGB_COLOR;
            continue;
        }
        if (!emit_operand(tr, stage_const, &a->t[k], a->one_minus[k], alpha_channel, &out->src[k], &out->op[k])) {
            return false;
        }
    }
    return true;
}

static bool emit_stage(Translator* tr, const ChannelProgram* rgb, const ChannelProgram* alpha)
{
    int count = rgb->count > alpha->count ? rgb->count : alpha->count;
    AbstractOp passthrough = op1(PICA_FUNC_REPLACE, term_src(PICA_SRC_PREVIOUS, false));
    for (int i = 0; i < count; i++) {
        if (tr->out->stage_count >= PICA_COMBINER_MAX_STAGES) {
            tr->reason = "more than 6 PICA stages needed";
            return false;
        }
        PicaCombinerStage* stage = &tr->out->stages[tr->out->stage_count];
        ConstSlot slot = { false, false, 0 };
        const AbstractOp* rgb_op = i < rgb->count ? &rgb->ops[i] : &passthrough;
        const AbstractOp* alpha_op = i < alpha->count ? &alpha->ops[i] : &passthrough;
        uint8_t rgb_scale = i == rgb->count - 1 ? rgb->scale : PICA_SCALE_1;
        uint8_t alpha_scale = i == alpha->count - 1 ? alpha->scale : PICA_SCALE_1;
        if (!emit_op(tr, &slot, rgb_op, false, rgb_scale, &stage->rgb) ||
            !emit_op(tr, &slot, alpha_op, true, alpha_scale, &stage->alpha)) {
            return false;
        }
        stage->constant = slot.value;
        tr->out->stage_count++;
    }
    return true;
}

static bool translate_stage(Translator* tr, const GxTevStage* s)
{
    bool has_previous = tr->out->stage_count > 0;
    Term color[4];
    Term alpha[4];
    for (int k = 0; k < 4; k++) {
        if (!color_term(tr, s, s->color.in[k], has_previous, &color[k]) ||
            !alpha_term(tr, s, s->alpha.in[k], has_previous, &alpha[k])) {
            return false;
        }
    }
    ChannelProgram rgb_prog;
    ChannelProgram alpha_prog;
    return build_channel(tr, &s->color, color, false, &rgb_prog) &&
           build_channel(tr, &s->alpha, alpha, true, &alpha_prog) &&
           emit_stage(tr, &rgb_prog, &alpha_prog);
}

static bool stage_reads_buffer(const PicaCombinerStage* st)
{
    for (int k = 0; k < 3; k++) { /* unused operands are PREVIOUS (emit_op) */
        if (st->rgb.src[k] == PICA_SRC_PREVIOUS_BUFFER || st->alpha.src[k] == PICA_SRC_PREVIOUS_BUFFER) {
            return true;
        }
    }
    return false;
}

/* The combiner buffer lags one stage: stage 0 reads it as 0 (Azahar's
 * sw_rasterizer.cpp, from hardware tests). If the first stage needs it, the
 * program moves down behind a pass-through stage, free as all six always run. */
static bool keep_buffer_out_of_stage0(Translator* tr)
{
    PicaCombiner* out = tr->out;
    if (out->stage_count == 0 || !stage_reads_buffer(&out->stages[0])) {
        return true;
    }
    if (out->stage_count >= PICA_COMBINER_MAX_STAGES) {
        tr->reason = "more than 6 PICA stages needed";
        return false;
    }
    memmove(&out->stages[1], &out->stages[0], sizeof(out->stages[0]) * (size_t)out->stage_count);
    PicaCombinerOp pass = { PICA_FUNC_REPLACE,
                            { PICA_SRC_PRIMARY, PICA_SRC_PRIMARY, PICA_SRC_PRIMARY },
                            { PICA_OP_RGB_COLOR, PICA_OP_RGB_COLOR, PICA_OP_RGB_COLOR },
                            PICA_SCALE_1 };
    out->stages[0].rgb = pass;
    out->stages[0].alpha = pass; /* PICA_OP_A_ALPHA == PICA_OP_RGB_COLOR == 0 */
    out->stages[0].constant = 0;
    out->stage_count++;
    return true;
}

bool gx_tev_translate(const GxTevConfig* gx, PicaCombiner* out, const char** reason)
{
    Translator tr = { gx, out, { false, false, 0 }, NULL };
    memset(out, 0, sizeof(*out));
    memset(out->unit_texmap, GX_TEV_NONE, sizeof(out->unit_texmap));

    bool ok = gx->stage_count >= 1 && gx->stage_count <= GX_TEV_MAX_STAGES;
    if (!ok) {
        tr.reason = "TEV stage count out of range";
    }
    for (int i = 0; ok && i < gx->stage_count; i++) {
        ok = translate_stage(&tr, &gx->stages[i]);
    }
    ok = ok && keep_buffer_out_of_stage0(&tr);
    out->uses_buffer_color = tr.buffer.rgb_set || tr.buffer.alpha_set;
    out->buffer_color = tr.buffer.value;
    if (!ok && reason != NULL) {
        *reason = tr.reason;
    }
    return ok;
}
