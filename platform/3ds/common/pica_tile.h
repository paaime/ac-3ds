#ifndef AC3DS_PICA_TILE_H
#define AC3DS_PICA_TILE_H

/* PICA200 texture layout: 8x8-texel tiles, row-major across the image; inside a
 * tile, Morton order with x lowest (x0 | y0<<1 | x1<<2 | y1<<3 | x2<<4 | y2<<5).
 * The PICA samples the first stored row at t = 1, so GC/N64 textures (row 0 at
 * t = 0) are uploaded with flip_rows = true and keep their coordinates. */

#include <stdbool.h>
#include <stdint.h>

#define PICA_TILE_DIM 8
#define PICA_TEXTURE_MAX_DIM 1024

/* `width` is a multiple of 8; `y` is the stored row (with flip_rows, image row
 * r is stored as row height - 1 - r). */
uint32_t pica_tiled_index(uint32_t x, uint32_t y, uint32_t width);

/* width and height: non-zero multiples of 8, at most PICA_TEXTURE_MAX_DIM; 1 to 4
 * bytes per texel; no overlap. False (dst untouched) on invalid arguments. */
bool pica_tile(void* dst, const void* src, uint32_t width, uint32_t height,
               uint32_t bytes_per_texel, bool flip_rows);

#endif /* AC3DS_PICA_TILE_H */
