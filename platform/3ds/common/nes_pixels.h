#ifndef AC3DS_NES_PIXELS_H
#define AC3DS_NES_PIXELS_H

#include <stdint.h>

#define NES_PIXELS_LUT_SIZE 256

/* Arguments as QuickNES's frame_t gives them (`colors`: RGB triplets); other indices black. */
void nes_pixels_lut(uint16_t lut[NES_PIXELS_LUT_SIZE], const int16_t* palette, int begin, int count,
                    const uint8_t* colors, int color_count);

/* `src_pitch` in bytes; `dst` rows packed. */
void nes_pixels_to_rgb565(uint16_t* dst, const uint8_t* src, long src_pitch, int width, int rows,
                          const uint16_t lut[NES_PIXELS_LUT_SIZE]);

#endif /* AC3DS_NES_PIXELS_H */
