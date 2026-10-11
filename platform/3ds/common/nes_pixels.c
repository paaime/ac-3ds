#include "nes_pixels.h"

#define RGB_BYTES 3

static uint16_t rgb565(const uint8_t* rgb)
{
    return (uint16_t)(((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3));
}

void nes_pixels_lut(uint16_t lut[NES_PIXELS_LUT_SIZE], const int16_t* palette, int begin, int count,
                    const uint8_t* colors, int color_count)
{
    for (int i = 0; i < NES_PIXELS_LUT_SIZE; i++) {
        lut[i] = 0;
        if (i >= begin && i - begin < count && palette[i] >= 0 && palette[i] < color_count) {
            lut[i] = rgb565(colors + palette[i] * RGB_BYTES);
        }
    }
}

void nes_pixels_to_rgb565(uint16_t* dst, const uint8_t* src, long src_pitch, int width, int rows,
                          const uint16_t lut[NES_PIXELS_LUT_SIZE])
{
    for (int y = 0; y < rows; y++) {
        const uint8_t* row = src + y * src_pitch;
        for (int x = 0; x < width; x++) {
            *dst++ = lut[row[x]];
        }
    }
}
