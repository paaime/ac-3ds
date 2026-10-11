#ifndef AC3DS_AUDIO_PACER_H
#define AC3DS_AUDIO_PACER_H

/* Paces the audio mixer from the game thread: each video frame, mix until the
 * samples produced catch up with the system clock (so a slow frame rate doesn't
 * slow the music), at most AUDIO_PACER_MAX_MIXES times. A debt over a second (a
 * loading stall) is dropped, not mixed in a burst. */

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PACER_MAX_MIXES 4

typedef struct {
    uint32_t sample_rate;
    uint64_t due;
    uint64_t produced;
} AudioPacer;

AudioPacer audio_pacer_create(uint32_t sample_rate);
AudioPacer audio_pacer_advance_to(AudioPacer p, uint64_t due_frames);
AudioPacer audio_pacer_produced(AudioPacer p, uint32_t frames);
bool audio_pacer_behind(AudioPacer p);

#endif /* AC3DS_AUDIO_PACER_H */
