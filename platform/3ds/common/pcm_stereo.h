#ifndef AC3DS_PCM_STEREO_H
#define AC3DS_PCM_STEREO_H

#include <stdint.h>

/* dst holds 2 * samples values and doesn't overlap src. */
void pcm_mono_to_stereo(int16_t* dst, const int16_t* src, uint32_t samples);

#endif /* AC3DS_PCM_STEREO_H */
