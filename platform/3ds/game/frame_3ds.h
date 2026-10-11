#ifndef AC3DS_FRAME_3DS_H
#define AC3DS_FRAME_3DS_H

/* citro3d frame lifecycle: the game's JW_BeginFrame and VIWaitForRetrace
 * (jsyswrap.cpp, TARGET_PC) open and submit a frame on the top screen; outside
 * a frame (loading loops) VIWaitForRetrace only waits for vblank. */

#include <stdbool.h>
#include <stdint.h>

#include "gx_render.h"

typedef struct {
    uint32_t frames;
    uint32_t idle_waits;   /* VIWaitForRetrace calls outside a frame */
    float cpu_ms;          /* citro3d CPU time of the last frame */
    float gpu_ms;          /* citro3d GPU time of the last frame */
    float cmdbuf_usage;    /* fraction of the command buffer the last frame used */
    GxrStats gx;           /* GX backend counters of the last frame */
} Frame3dsStats;

/* citro3d, the top render target and the GX backend. */
bool frame3ds_init(void);
void frame3ds_shutdown(void);

void frame3ds_begin(void);
void frame3ds_end(void);
bool frame3ds_is_open(void);

Frame3dsStats frame3ds_stats(void);

#endif /* AC3DS_FRAME_3DS_H */
