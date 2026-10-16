#include "audio_pacer.h"

#define RESYNC_OWED_DIVISOR 60 /* after a stall, owe one video frame (1/60 s) */

AudioPacer audio_pacer_create(uint32_t sample_rate)
{
    AudioPacer p = { sample_rate, 0, 0 };
    return p;
}

AudioPacer audio_pacer_advance_to(AudioPacer p, uint64_t due_frames)
{
    AudioPacer next = p;
    if (due_frames > next.due) {
        next.due = due_frames;
    }
    uint64_t max_lag = next.sample_rate;
    if (next.due > next.produced && next.due - next.produced > max_lag) {
        next.produced = next.due - next.sample_rate / RESYNC_OWED_DIVISOR;
    }
    return next;
}

AudioPacer audio_pacer_produced(AudioPacer p, uint32_t frames)
{
    AudioPacer next = p;
    next.produced += frames;
    return next;
}

bool audio_pacer_behind(AudioPacer p)
{
    return p.produced < p.due;
}
