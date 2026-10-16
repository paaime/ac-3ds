#ifndef AC3DS_PROF_WINDOW_H
#define AC3DS_PROF_WINDOW_H

#include <stdint.h>

#define PROF_WINDOW_SLOTS 16

typedef struct {
    uint64_t ticks[PROF_WINDOW_SLOTS];
    uint64_t frame_ticks;
    uint32_t frames;
} ProfWindow;

ProfWindow prof_window_create(void);
ProfWindow prof_window_add(ProfWindow w, unsigned slot, uint64_t ticks);
ProfWindow prof_window_end_frame(ProfWindow w, uint64_t frame_ticks);

uint32_t prof_window_frames(ProfWindow w);
double prof_window_ms(ProfWindow w, unsigned slot, uint32_t tick_hz);
double prof_window_frame_ms(ProfWindow w, uint32_t tick_hz);

double prof_share_percent(uint64_t part, uint64_t whole);

#endif /* AC3DS_PROF_WINDOW_H */
