#ifndef AC3DS_AUDIO_3DS_H
#define AC3DS_AUDIO_3DS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool output_on;        /* ndsp is playing the mixer's output */
    int mixer_core;        /* the mixer thread's core, or -1 (game thread) */
    uint32_t queued_ms;    /* audio waiting to play */
    uint32_t underruns;    /* times the output ran dry */
    uint32_t dropped;      /* chunks dropped: every buffer busy */
    uint32_t level_percent; /* loudest sample since the last status, % of full scale */
} Audio3dsStatus;

/* Once per video frame: runs the mixer when no mixer thread does. */
void audio3ds_frame(void);

Audio3dsStatus audio3ds_status(void);

#endif /* AC3DS_AUDIO_3DS_H */
