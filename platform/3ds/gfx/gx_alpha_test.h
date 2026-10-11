#ifndef AC3DS_GX_ALPHA_TEST_H
#define AC3DS_GX_ALPHA_TEST_H

/* GX alpha compare -> PICA200 alpha test. GX tests alpha twice, (comp0 ref0) op
 * (comp1 ref1); the PICA once. With AND, two lower (or upper) bounds merge
 * exactly into the stricter one, which covers emu64;
 * other combinations are approximated and flagged. */

#include <stdbool.h>
#include <stdint.h>

/* GXCompare and GXAlphaOp values (GPU_TESTFUNC is ordered differently). */
enum {
    GXA_NEVER, GXA_LESS, GXA_EQUAL, GXA_LEQUAL, GXA_GREATER, GXA_NOTEQUAL, GXA_GEQUAL, GXA_ALWAYS,
};
enum { GXA_OP_AND, GXA_OP_OR, GXA_OP_XOR, GXA_OP_XNOR };

typedef struct {
    bool enable;  /* false: every fragment passes */
    uint8_t func; /* GXA_* compare, when enabled */
    uint8_t ref;
    bool exact;   /* false: an approximation of the GX test */
} GxAlphaTest;

GxAlphaTest gx_alpha_test_merge(uint8_t comp0, uint8_t ref0, uint8_t op, uint8_t comp1, uint8_t ref1);

#endif /* AC3DS_GX_ALPHA_TEST_H */
