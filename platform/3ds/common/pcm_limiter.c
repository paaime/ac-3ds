#include "pcm_limiter.h"

#include <math.h>

#define HOLD_SECONDS    0.025f /* a 40 Hz period: the gain stays put through a bass note's cycle */
#define RELEASE_SECONDS 0.15f

PcmLimiter pcm_limiter_create(float gain, float ceiling, float sample_rate)
{
    PcmLimiter l = { gain, ceiling, expf(-1.0f / (RELEASE_SECONDS * sample_rate)),
                     (uint32_t)lrintf(HOLD_SECONDS * sample_rate), 0.0f, 0 };
    return l;
}

static float frame_peak(const int16_t* frame)
{
    float left = fabsf((float)frame[0]);
    float right = fabsf((float)frame[1]);
    return left > right ? left : right;
}

static PcmLimiter follow(PcmLimiter l, float boosted)
{
    if (boosted >= l.envelope) {
        l.envelope = boosted;
        l.hold_left = l.hold_frames;
    } else if (l.hold_left > 0) {
        l.hold_left--;
    } else {
        float decayed = l.envelope * l.release;
        l.envelope = decayed > boosted ? decayed : boosted;
    }
    return l;
}

PcmLimiter pcm_limiter_run(PcmLimiter l, int16_t* dst, const int16_t* src, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; i++) {
        const int16_t* in = src + 2 * i;
        l = follow(l, frame_peak(in) * l.gain);
        float k = l.envelope > l.ceiling ? l.gain * l.ceiling / l.envelope : l.gain;
        int16_t left = (int16_t)lrintf(in[0] * k);
        int16_t right = (int16_t)lrintf(in[1] * k);
        dst[2 * i] = left;
        dst[2 * i + 1] = right;
    }
    return l;
}
