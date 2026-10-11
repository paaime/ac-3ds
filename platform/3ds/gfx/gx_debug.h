#ifndef AC3DS_GX_DEBUG_H
#define AC3DS_GX_DEBUG_H

/* GX frame dump: after gxd_arm(), each draw of the next whole frame is logged to
 * stderr (ac3ds.log) on one line. The developer build arms it with SELECT + L. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    uint8_t primitive;
    uint32_t count;
    const float* first_pos; /* object-space position of the first vertex */
    const uint8_t* first_color;
    int pica_stages;  /* the combiner's stages, the fallback's too; -1 if not drawn */
    const char* fate; /* "draw", or why it wasn't drawn */
} GxdDraw;

void gxd_arm(void);
void gxd_frame_begin(void);
void gxd_frame_end(void);
bool gxd_active(void);
void gxd_draw(const GxbState* st, const GxdDraw* d);

#endif /* AC3DS_GX_DEBUG_H */
