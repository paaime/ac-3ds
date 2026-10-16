#ifndef AC3DS_GX_ALPHA_TEST_H
#define AC3DS_GX_ALPHA_TEST_H

#include <stdbool.h>
#include <stdint.h>

/* GXCompare and GXAlphaOp values (GPU_TESTFUNC is ordered differently). */
enum {
    GXA_NEVER, GXA_LESS, GXA_EQUAL, GXA_LEQUAL, GXA_GREATER, GXA_NOTEQUAL, GXA_GEQUAL, GXA_ALWAYS,
};
enum { GXA_OP_AND, GXA_OP_OR, GXA_OP_XOR, GXA_OP_XNOR };

typedef struct {
    bool enable;
    uint8_t func;
    uint8_t ref;
    bool exact;
} GxAlphaTest;

GxAlphaTest gx_alpha_test_merge(uint8_t comp0, uint8_t ref0, uint8_t op, uint8_t comp1, uint8_t ref1);

#endif /* AC3DS_GX_ALPHA_TEST_H */
