#ifndef AC3DS_AUDIO_THREAD_3DS_H
#define AC3DS_AUDIO_THREAD_3DS_H

#include <stdbool.h>

typedef void (*AudioFillFn)(void);

/* False if core 2 isn't available. */
bool audio_thread_start(AudioFillFn fill);
void audio_thread_stop(void);
/* -1 if it isn't running. */
int audio_thread_core(void);

#endif /* AC3DS_AUDIO_THREAD_3DS_H */
