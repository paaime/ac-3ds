#ifndef AC3DS_PCM_STEREO_H
#define AC3DS_PCM_STEREO_H

/* Mono PCM16 to interleaved stereo, the same on both sides: QuickNES's sound is
 * mono, the NES channel's stream stereo (game/nes_audio_3ds.c). */

#include <stdint.h>

/* dst holds 2 * samples values and doesn't overlap src. */
void pcm_mono_to_stereo(int16_t* dst, const int16_t* src, uint32_t samples);

#endif /* AC3DS_PCM_STEREO_H */
