#include "gx_alpha_test.h"

#define ALPHA_MAX 255

typedef enum {
    KIND_ALWAYS,
    KIND_NEVER,
    KIND_LOWER, /* alpha >= threshold */
    KIND_UPPER, /* alpha <= threshold */
    KIND_OTHER, /* EQUAL, NOTEQUAL */
    KIND_INVALID,
} BoundKind;

typedef struct {
    BoundKind kind;
    int threshold;
    uint8_t func;
    uint8_t ref;
} Bound;

/* One GX compare as a bound on alpha; trivially true/false ones are folded. */
static Bound classify(uint8_t func, uint8_t ref)
{
    Bound b = { KIND_OTHER, 0, func, ref };
    switch (func) {
    case GXA_ALWAYS: b.kind = KIND_ALWAYS; break;
    case GXA_NEVER: b.kind = KIND_NEVER; break;
    case GXA_GEQUAL: b.kind = KIND_LOWER; b.threshold = ref; break;
    case GXA_GREATER: b.kind = KIND_LOWER; b.threshold = ref + 1; break;
    case GXA_LEQUAL: b.kind = KIND_UPPER; b.threshold = ref; break;
    case GXA_LESS: b.kind = KIND_UPPER; b.threshold = ref - 1; break;
    case GXA_EQUAL:
    case GXA_NOTEQUAL: break;
    default: b.kind = KIND_INVALID; break;
    }
    if (b.kind == KIND_LOWER) {
        b.kind = b.threshold <= 0 ? KIND_ALWAYS : (b.threshold > ALPHA_MAX ? KIND_NEVER : KIND_LOWER);
    } else if (b.kind == KIND_UPPER) {
        b.kind = b.threshold >= ALPHA_MAX ? KIND_ALWAYS : (b.threshold < 0 ? KIND_NEVER : KIND_UPPER);
    }
    return b;
}

static GxAlphaTest single(const Bound* b, bool exact)
{
    GxAlphaTest t = { true, b->func, b->ref, exact };
    if (b->kind == KIND_ALWAYS) {
        t.enable = false;
    } else if (b->kind == KIND_NEVER) {
        t.func = GXA_NEVER;
        t.ref = 0;
    }
    return t;
}

static GxAlphaTest merge_and(const Bound* b0, const Bound* b1)
{
    if (b0->kind == KIND_LOWER && b1->kind == KIND_LOWER) {
        return single(b0->threshold >= b1->threshold ? b0 : b1, true); /* the stricter */
    }
    if (b0->kind == KIND_UPPER && b1->kind == KIND_UPPER) {
        return single(b0->threshold <= b1->threshold ? b0 : b1, true);
    }
    if (b0->kind == KIND_UPPER && b1->kind == KIND_LOWER) {
        return single(b1, false); /* a range: keep the lower bound (cut-out edges) */
    }
    return single(b0, false); /* a range with b0 the lower bound, or EQUAL/NOTEQUAL */
}

GxAlphaTest gx_alpha_test_merge(uint8_t comp0, uint8_t ref0, uint8_t op, uint8_t comp1, uint8_t ref1)
{
    static const GxAlphaTest DISABLED_APPROX = { false, GXA_ALWAYS, 0, false };
    Bound b0 = classify(comp0, ref0);
    Bound b1 = classify(comp1, ref1);
    if (op != GXA_OP_AND || b0.kind == KIND_INVALID || b1.kind == KIND_INVALID) {
        return DISABLED_APPROX; /* emu64 only uses AND */
    }
    if (b0.kind == KIND_NEVER || b1.kind == KIND_ALWAYS) {
        return single(&b0, true);
    }
    if (b1.kind == KIND_NEVER || b0.kind == KIND_ALWAYS) {
        return single(&b1, true);
    }
    return merge_and(&b0, &b1);
}
