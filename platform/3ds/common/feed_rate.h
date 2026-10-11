#ifndef AC3DS_FEED_RATE_H
#define AC3DS_FEED_RATE_H

/* The rate NES sound really arrives at, over ~0.5 s windows, clamped near
 * nominal: it absorbs drift and jitter, while a big slowdown gives gaps rather
 * than a deep pitch drop. A pause restarts the window. */

#include <stdint.h>

#define FEED_RATE_MIN_RATIO 0.9f
#define FEED_RATE_MAX_RATIO 1.05f

typedef struct {
    float nominal;
    float current;
    uint32_t tick_hz;
    uint64_t window_start; /* tick */
    uint64_t last_tick;
    uint64_t window_frames;
    int started;
} FeedRate;

FeedRate feed_rate_create(float nominal, uint32_t tick_hz);
FeedRate feed_rate_add(FeedRate r, uint32_t frames, uint64_t now);
float feed_rate_value(FeedRate r);

#endif /* AC3DS_FEED_RATE_H */
