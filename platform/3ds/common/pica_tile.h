#ifndef AC3DS_PICA_TILE_H
#define AC3DS_PICA_TILE_H

#include <stdbool.h>
#include <stdint.h>

#define PICA_TILE_DIM 8
#define PICA_TEXTURE_MAX_DIM 1024

/* 8x8 tiles, Morton order inside (x lowest); `width` a multiple of 8, `y` the stored row. */
uint32_t pica_tiled_index(uint32_t x, uint32_t y, uint32_t width);

/* The PICA samples the first stored row at t = 1: GC/N64 textures (row 0 at t = 0) need flip_rows. */
bool pica_tile(void* dst, const void* src, uint32_t width, uint32_t height,
               uint32_t bytes_per_texel, bool flip_rows);

#endif /* AC3DS_PICA_TILE_H */
