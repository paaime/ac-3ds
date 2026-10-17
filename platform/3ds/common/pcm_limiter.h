#ifndef AC3DS_PCM_LIMITER_H
#define AC3DS_PCM_LIMITER_H

#include <stdint.h>

typedef struct {
    float gain;
    float ceiling;
    float release;
    uint32_t hold_frames;
    float envelope;
    uint32_t hold_left;
} PcmLimiter;

/* `ceiling` in sample units (at most 32767). */
PcmLimiter pcm_limiter_create(float gain, float ceiling, float sample_rate);
/* Interleaved stereo, both channels turned down together; dst may be src. */
PcmLimiter pcm_limiter_run(PcmLimiter l, int16_t* dst, const int16_t* src, uint32_t frames);

#endif /* AC3DS_PCM_LIMITER_H */
