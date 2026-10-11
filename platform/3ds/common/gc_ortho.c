#include "gc_ortho.h"

void gc_ortho(float m[4][4], float top, float bottom, float left, float right, float near_z, float far_z)
{
    float w = 1.0f / (right - left);
    float h = 1.0f / (top - bottom);
    float d = 1.0f / (far_z - near_z);
    const float rows[4][4] = {
        { 2.0f * w, 0.0f, 0.0f, -(right + left) * w },
        { 0.0f, 2.0f * h, 0.0f, -(top + bottom) * h },
        { 0.0f, 0.0f, -d, -far_z * d },
        { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            m[r][c] = rows[r][c];
        }
    }
}
