#ifndef AC3DS_GX_DEBUG_H
#define AC3DS_GX_DEBUG_H

#include <stdbool.h>
#include <stdint.h>

#include "gx_state.h"

typedef struct {
    uint8_t primitive;
    uint32_t count;
    const float* first_pos;
    const uint8_t* first_color;
    int pica_stages; /* -1 if not drawn */
    const char* fate;
} GxdDraw;

/* Logs each draw of the next whole frame to stderr (ac3ds.log). */
void gxd_arm(void);
void gxd_frame_begin(void);
void gxd_frame_end(void);
bool gxd_active(void);
void gxd_draw(const GxbState* st, const GxdDraw* d);

#endif /* AC3DS_GX_DEBUG_H */
