#ifndef AC3DS_NES_AUDIO_3DS_H
#define AC3DS_NES_AUDIO_3DS_H

#include <stdint.h>

/* Game thread, one NES frame per call; silent without ndsp. */
void nes_audio_write(const int16_t* mono, uint32_t samples, uint32_t sample_rate);
void nes_audio_stop(void);

#endif /* AC3DS_NES_AUDIO_3DS_H */
