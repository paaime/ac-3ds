#ifndef AC3DS_NES_PIXELS_H
#define AC3DS_NES_PIXELS_H

/* QuickNES's picture in RGB565 (game/nes_3ds.cpp): it draws 8-bit host palette
 * indices, and each frame says which colour of its NES table each index shows. */

#include <stdint.h>

#define NES_PIXELS_LUT_SIZE 256

/* lut[i] = colour palette[i] of `colors` (RGB triplets, color_count of them)
 * for begin <= i < begin + count, as QuickNES's frame_t gives them; black for
 * the other indices and for colours out of the table. RGB565: red in the high
 * bits (the PICA's GPU_RGB565). */
void nes_pixels_lut(uint16_t lut[NES_PIXELS_LUT_SIZE], const int16_t* palette, int begin, int count,
                    const uint8_t* colors, int color_count);

/* `rows` rows of `width` indices, `src_pitch` bytes apart, to packed RGB565 rows. */
void nes_pixels_to_rgb565(uint16_t* dst, const uint8_t* src, long src_pitch, int width, int rows,
                          const uint16_t lut[NES_PIXELS_LUT_SIZE]);

#endif /* AC3DS_NES_PIXELS_H */
