#include "feed_rate.h"

#define WINDOW_DIVISOR 2
#define STALL_SECONDS  1

FeedRate feed_rate_create(float nominal, uint32_t tick_hz)
{
    FeedRate r = { nominal, nominal, tick_hz, 0, 0, 0, 0 };
    return r;
}

static float clamp_rate(float rate, float nominal)
{
    float lo = nominal * FEED_RATE_MIN_RATIO;
    float hi = nominal * FEED_RATE_MAX_RATIO;
    return rate < lo ? lo : (rate > hi ? hi : rate);
}

FeedRate feed_rate_add(FeedRate r, uint32_t frames, uint64_t now)
{
    if (!r.started || now - r.last_tick > (uint64_t)STALL_SECONDS * r.tick_hz) {
        r.started = 1;
        r.window_start = now; /* these frames were produced before the clock started */
        r.window_frames = 0;
        r.last_tick = now;
        return r;
    }
    r.last_tick = now;
    r.window_frames += frames;
    uint64_t elapsed = now - r.window_start;
    if (elapsed >= r.tick_hz / WINDOW_DIVISOR) {
        float measured = (float)((double)r.window_frames * r.tick_hz / (double)elapsed);
        r.current = clamp_rate(measured, r.nominal);
        r.window_start = now;
        r.window_frames = 0;
    }
    return r;
}

float feed_rate_value(FeedRate r)
{
    return r.current;
}
