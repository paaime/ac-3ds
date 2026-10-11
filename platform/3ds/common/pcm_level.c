#include "pcm_level.h"

#define FULL_SCALE 32768u
#define PERCENT    100u

uint32_t pcm_peak(const int16_t* samples, uint32_t count)
{
    uint32_t peak = 0;
    for (uint32_t i = 0; i < count; i++) {
        int32_t s = samples[i];
        uint32_t magnitude = (uint32_t)(s < 0 ? -s : s);
        peak = magnitude > peak ? magnitude : peak;
    }
    return peak;
}

uint32_t pcm_peak_percent(uint32_t peak)
{
    return (peak * PERCENT + FULL_SCALE - 1) / FULL_SCALE;
}
