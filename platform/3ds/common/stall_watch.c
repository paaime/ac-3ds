#include "stall_watch.h"

StallWatch stall_watch_start(uint32_t seq, uint64_t now_ms)
{
    StallWatch w = { seq, now_ms, 0, false, STALL_NONE };
    return w;
}

StallWatch stall_watch_check(StallWatch w, uint32_t seq, uint64_t now_ms, uint64_t limit_ms, uint64_t repeat_ms)
{
    if (seq != w.seq) {
        StallWatch moved = stall_watch_start(seq, now_ms);
        moved.event = w.reported ? STALL_ENDED : STALL_NONE;
        return moved;
    }
    StallWatch next = w;
    next.event = STALL_NONE;
    if (now_ms < w.since_ms) {
        return next; /* the clock went back: wait for it */
    }
    if (!w.reported && now_ms - w.since_ms >= limit_ms) {
        next.reported = true;
        next.reported_ms = now_ms;
        next.event = STALL_BEGAN;
    } else if (w.reported && now_ms - w.reported_ms >= repeat_ms) {
        next.reported_ms = now_ms;
        next.event = STALL_GOES_ON;
    }
    return next;
}
