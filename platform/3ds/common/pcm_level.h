#ifndef AC3DS_PCM_LEVEL_H
#define AC3DS_PCM_LEVEL_H

#include <stdint.h>

/* 0..32768. */
uint32_t pcm_peak(const int16_t* samples, uint32_t count);

/* Rounded up, so any signal reads >= 1. */
uint32_t pcm_peak_percent(uint32_t peak);

#endif /* AC3DS_PCM_LEVEL_H */
