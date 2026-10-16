#ifndef AC3DS_FRAME_3DS_H
#define AC3DS_FRAME_3DS_H

#include <stdbool.h>
#include <stdint.h>

#include "gx_render.h"

typedef struct {
    uint32_t frames;
    uint32_t idle_waits;
    float cpu_ms;
    float gpu_ms;
    float cmdbuf_usage; /* 0..1 */
    GxrStats gx;
} Frame3dsStats;

bool frame3ds_init(void);
void frame3ds_shutdown(void);

/* Called by JW_BeginFrame and VIWaitForRetrace (jsyswrap.cpp, TARGET_PC). */
void frame3ds_begin(void);
void frame3ds_end(void);
bool frame3ds_is_open(void);

Frame3dsStats frame3ds_stats(void);

#endif /* AC3DS_FRAME_3DS_H */
