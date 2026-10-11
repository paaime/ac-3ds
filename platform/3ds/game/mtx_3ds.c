/* mtx_3ds.c - the Dolphin SDK's C_MTXOrtho (common/gc_ortho.h). pc_mtx.c's copy
 * (renamed away in CMakeLists.txt) maps depth to [0, 1], which puts emu64's 2D
 * rectangles outside the clip volume the PICA shares with GX. */
#include "gc_ortho.h"

void C_MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f);

void C_MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f)
{
    gc_ortho(m, t, b, l, r, n, f);
}
