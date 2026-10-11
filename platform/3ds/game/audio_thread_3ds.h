#ifndef AC3DS_AUDIO_THREAD_3DS_H
#define AC3DS_AUDIO_THREAD_3DS_H

/* The audio mixer thread on the New 3DS's core 2 (audio_thread_3ds.c).
 * Never on the game's core: at a higher priority, a mixer falling behind would
 * starve the game thread. */

#include <stdbool.h>

typedef void (*AudioFillFn)(void);

/* False if core 2 isn't available; the caller then fills from the game thread. */
bool audio_thread_start(AudioFillFn fill);
void audio_thread_stop(void);
/* The core the thread runs on, or -1 if it isn't running. */
int audio_thread_core(void);

#endif /* AC3DS_AUDIO_THREAD_3DS_H */
