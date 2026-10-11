#ifndef AC3DS_PROFILER_3DS_H
#define AC3DS_PROFILER_3DS_H

/* Where each frame's CPU time goes (profiler_3ds.c): the
 * game's own PC-profiler timers (graph.c, TARGET_PC) plus the 3DS-only parts. */

#include <stdint.h>

typedef enum {
    PROF_LOGIC,       /* game_main */
    PROF_DRAW_FINISH, /* graph_draw_finish: closing the display lists */
    PROF_EMU64,       /* emu64_taskstart: display lists -> GX backend -> citro3d */
    PROF_FRAME_WAIT,  /* C3D_FrameBegin: waiting for the GPU and the frame slot */
    PROF_FRAME_END,   /* C3D_FrameEnd */
    PROF_FLUSH,       /* the CPU cache flush before submission */
    PROF_TEXTURES,    /* texture decode and tiling (texture cache misses) */
    PROF_AUDIO_GAME,  /* the game's per-frame sound calls (Na_GameFrame) */
    PROF_STATUS,
    PROF_COUNT
} Prof3dsSlot;

typedef struct {
    float ms[PROF_COUNT]; /* average per frame */
    float frame_ms;       /* average whole frame */
    float mixer_percent;  /* share of core 2 the sound mixer used */
    uint32_t frames;      /* frames in the window (0 until the first one closes) */
} Prof3dsReport;

uint64_t prof3ds_now(void);
/* Adds the time since `start` (a prof3ds_now value) to `slot`. */
void prof3ds_add(Prof3dsSlot slot, uint64_t start);
void prof3ds_add_ticks(Prof3dsSlot slot, uint64_t ticks);
/* Called from the mixer thread (core 2). */
void prof3ds_mixer_add(uint32_t ticks);
/* Once per frame, at its very end (VIWaitForRetrace). */
void prof3ds_frame(void);
/* The last completed window. */
Prof3dsReport prof3ds_report(void);

#endif /* AC3DS_PROFILER_3DS_H */
