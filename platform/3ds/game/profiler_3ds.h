#ifndef AC3DS_PROFILER_3DS_H
#define AC3DS_PROFILER_3DS_H

#include <stdint.h>

typedef enum {
    PROF_LOGIC,
    PROF_DRAW_FINISH,
    PROF_EMU64,
    PROF_FRAME_WAIT,
    PROF_FRAME_END,
    PROF_FLUSH,
    PROF_TEXTURES,
    PROF_AUDIO_GAME,
    PROF_STATUS,
    PROF_COUNT
} Prof3dsSlot;

typedef struct {
    float ms[PROF_COUNT]; /* average per frame */
    float frame_ms;
    float mixer_percent;
    uint32_t frames; /* 0 until the first window closes */
} Prof3dsReport;

uint64_t prof3ds_now(void);
void prof3ds_add(Prof3dsSlot slot, uint64_t start);
void prof3ds_add_ticks(Prof3dsSlot slot, uint64_t ticks);
/* Called from the mixer thread (core 2). */
void prof3ds_mixer_add(uint32_t ticks);
void prof3ds_frame(void);
Prof3dsReport prof3ds_report(void);

#endif /* AC3DS_PROFILER_3DS_H */
