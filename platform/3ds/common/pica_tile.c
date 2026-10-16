#include "pica_tile.h"

#include <stddef.h>
#include <string.h>

#define TEXELS_PER_TILE (PICA_TILE_DIM * PICA_TILE_DIM)

static uint32_t spread3(uint32_t v)
{
    return (v & 1u) | ((v & 2u) << 1) | ((v & 4u) << 2);
}

uint32_t pica_tiled_index(uint32_t x, uint32_t y, uint32_t width)
{
    uint32_t tiles_per_row = width / PICA_TILE_DIM;
    uint32_t tile = (y / PICA_TILE_DIM) * tiles_per_row + (x / PICA_TILE_DIM);
    uint32_t in_tile = spread3(x % PICA_TILE_DIM) | (spread3(y % PICA_TILE_DIM) << 1);
    return tile * TEXELS_PER_TILE + in_tile;
}

static bool is_valid_side(uint32_t side)
{
    return side != 0 && side % PICA_TILE_DIM == 0 && side <= PICA_TEXTURE_MAX_DIM;
}

static bool ranges_overlap(const void* a, const void* b, size_t size)
{
    uintptr_t pa = (uintptr_t)a;
    uintptr_t pb = (uintptr_t)b;
    return pa < pb + size && pb < pa + size;
}

static bool arguments_valid(const void* dst, const void* src, uint32_t width, uint32_t height,
                            uint32_t bytes_per_texel)
{
    if (dst == NULL || src == NULL || !is_valid_side(width) || !is_valid_side(height)) {
        return false;
    }
    if (bytes_per_texel < 1 || bytes_per_texel > 4) {
        return false;
    }
    return !ranges_overlap(dst, src, (size_t)width * height * bytes_per_texel);
}

/* Called with a literal bytes_per_texel so the compiler specialises the copy. */
static inline void convert(uint8_t* dst, const uint8_t* src, uint32_t width, uint32_t height,
                           uint32_t bytes_per_texel, bool flip_rows)
{
    static const uint8_t SPREAD_X[PICA_TILE_DIM] = { 0, 1, 4, 5, 16, 17, 20, 21 }; /* spread3(x) */
    size_t tile_row_texels = (size_t)width * PICA_TILE_DIM;
    for (uint32_t y = 0; y < height; y++) {
        uint32_t stored_row = flip_rows ? height - 1 - y : y;
        uint8_t* row = dst + ((stored_row / PICA_TILE_DIM) * tile_row_texels +
                              (spread3(stored_row % PICA_TILE_DIM) << 1)) * bytes_per_texel;
        const uint8_t* from = src + (size_t)y * width * bytes_per_texel;
        for (uint32_t tile_x = 0; tile_x < width; tile_x += PICA_TILE_DIM) {
            uint8_t* tile = row + (size_t)tile_x * PICA_TILE_DIM * bytes_per_texel;
            for (uint32_t i = 0; i < PICA_TILE_DIM; i++) {
                memcpy(tile + SPREAD_X[i] * bytes_per_texel, from, bytes_per_texel);
                from += bytes_per_texel;
            }
        }
    }
}

bool pica_tile(void* dst, const void* src, uint32_t width, uint32_t height,
               uint32_t bytes_per_texel, bool flip_rows)
{
    if (!arguments_valid(dst, src, width, height, bytes_per_texel)) {
        return false;
    }
    switch (bytes_per_texel) {
    case 1: convert(dst, src, width, height, 1, flip_rows); break;
    case 2: convert(dst, src, width, height, 2, flip_rows); break;
    case 3: convert(dst, src, width, height, 3, flip_rows); break;
    default: convert(dst, src, width, height, 4, flip_rows); break;
    }
    return true;
}

