#ifndef AC3DS_AUDIO_3DS_H
#define AC3DS_AUDIO_3DS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool output_on;
    int mixer_core; /* -1: no mixer thread, the game thread mixes */
    uint32_t queued_ms;
    uint32_t underruns;
    uint32_t dropped;
    uint32_t level_percent; /* loudest sample since the last call */
} Audio3dsStatus;

/* Once per video frame; runs the mixer when no mixer thread does. */
void audio3ds_frame(void);

Audio3dsStatus audio3ds_status(void);

#endif /* AC3DS_AUDIO_3DS_H */
