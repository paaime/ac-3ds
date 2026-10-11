#include "pcm_stereo.h"

void pcm_mono_to_stereo(int16_t* dst, const int16_t* src, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; i++) {
        dst[2 * i] = src[i];
        dst[2 * i + 1] = src[i];
    }
}
