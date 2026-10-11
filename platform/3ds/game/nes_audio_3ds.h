#ifndef AC3DS_NES_AUDIO_3DS_H
#define AC3DS_NES_AUDIO_3DS_H

/* NES sound on its own ndsp channel (nes_audio_3ds.c),
 * fed from the game thread one NES frame per call; silent without ndsp. */

#include <stdint.h>

void nes_audio_write(const int16_t* mono, uint32_t samples, uint32_t sample_rate);
void nes_audio_stop(void);

#endif /* AC3DS_NES_AUDIO_3DS_H */
