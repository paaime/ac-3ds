#ifndef AC3DS_GC_PATTERN_H
#define AC3DS_GC_PATTERN_H

/* Textures the port draws itself, in GX RGB5A3 (big-endian, 4x4-texel blocks).
 * Sizes must be multiples of GC_PATTERN_BLOCK and `out` at least
 * gc_rgb5a3_size bytes; false (out untouched) otherwise. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GC_PATTERN_BLOCK 4

uint16_t gc_rgb5a3_opaque(uint8_t r, uint8_t g, uint8_t b);

size_t gc_rgb5a3_size(uint32_t width, uint32_t height);

/* Cells of square x square texels, `first` at the top-left. */
bool gc_checker_rgb5a3(uint8_t* out, size_t out_size, uint32_t width, uint32_t height, uint32_t square,
                       uint16_t first, uint16_t second);

/* Opaque inside, fading to clear over about a texel and a half at the edge. */
bool gc_disc_rgb5a3(uint8_t* out, size_t out_size, uint32_t dim, uint8_t r, uint8_t g, uint8_t b);

/* Tip a quarter of the way down (or up), base at three quarters, three quarters
 * of the width wide; soft edges. */
bool gc_arrow_rgb5a3(uint8_t* out, size_t out_size, uint32_t dim, bool up, uint8_t r, uint8_t g, uint8_t b);

#endif /* AC3DS_GC_PATTERN_H */
