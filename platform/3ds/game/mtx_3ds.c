#include "gc_ortho.h"

/* pc_mtx.c's copy (renamed away) maps depth to [0, 1], which clips emu64's 2D rectangles on the PICA. */
void C_MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f);

void C_MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f)
{
    gc_ortho(m, t, b, l, r, n, f);
}
