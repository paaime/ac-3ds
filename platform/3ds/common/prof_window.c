#include "prof_window.h"

#include <string.h>

#define MS_PER_SECOND 1000.0
#define PERCENT       100.0

ProfWindow prof_window_create(void)
{
    ProfWindow w;
    memset(&w, 0, sizeof(w));
    return w;
}

ProfWindow prof_window_add(ProfWindow w, unsigned slot, uint64_t ticks)
{
    if (slot < PROF_WINDOW_SLOTS) {
        w.ticks[slot] += ticks;
    }
    return w;
}

ProfWindow prof_window_end_frame(ProfWindow w, uint64_t frame_ticks)
{
    w.frame_ticks += frame_ticks;
    w.frames++;
    return w;
}

uint32_t prof_window_frames(ProfWindow w)
{
    return w.frames;
}

static double per_frame_ms(uint64_t ticks, uint32_t frames, uint32_t tick_hz)
{
    if (frames == 0 || tick_hz == 0) {
        return 0.0;
    }
    return (double)ticks * MS_PER_SECOND / (double)tick_hz / (double)frames;
}

double prof_window_ms(ProfWindow w, unsigned slot, uint32_t tick_hz)
{
    return slot < PROF_WINDOW_SLOTS ? per_frame_ms(w.ticks[slot], w.frames, tick_hz) : 0.0;
}

double prof_window_frame_ms(ProfWindow w, uint32_t tick_hz)
{
    return per_frame_ms(w.frame_ticks, w.frames, tick_hz);
}

double prof_share_percent(uint64_t part, uint64_t whole)
{
    return whole == 0 ? 0.0 : (double)part * PERCENT / (double)whole;
}
