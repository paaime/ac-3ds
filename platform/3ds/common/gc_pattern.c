#include "gc_pattern.h"

#include <math.h>

#define RGB5A3_OPAQUE   0x8000u
#define BYTES_PER_TEXEL 2u
#define TEXELS_PER_BLOCK (GC_PATTERN_BLOCK * GC_PATTERN_BLOCK)
#define ALPHA3_MAX      7 /* RGB5A3's translucent texels: 3-bit alpha, 4-bit colour */
#define DISC_EDGE       1.5f
#define ARROW_TIP       0.25f
#define ARROW_BASE      0.75f
#define ARROW_HALF_W    0.375f
#define ARROW_EDGE      1.0f

uint16_t gc_rgb5a3_opaque(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(RGB5A3_OPAQUE | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

size_t gc_rgb5a3_size(uint32_t width, uint32_t height)
{
    return (size_t)width * height * BYTES_PER_TEXEL;
}

static size_t texel_offset(uint32_t width, uint32_t x, uint32_t y)
{
    uint32_t block = (y / GC_PATTERN_BLOCK) * (width / GC_PATTERN_BLOCK) + x / GC_PATTERN_BLOCK;
    uint32_t inside = (y % GC_PATTERN_BLOCK) * GC_PATTERN_BLOCK + x % GC_PATTERN_BLOCK;
    return ((size_t)block * TEXELS_PER_BLOCK + inside) * BYTES_PER_TEXEL;
}

static bool block_size(uint32_t width, uint32_t height)
{
    return width != 0 && height != 0 && width % GC_PATTERN_BLOCK == 0 && height % GC_PATTERN_BLOCK == 0;
}

static void put(uint8_t* out, uint32_t width, uint32_t x, uint32_t y, uint16_t c)
{
    uint8_t* p = out + texel_offset(width, x, y);
    p[0] = (uint8_t)(c >> 8);
    p[1] = (uint8_t)(c & 0xFF);
}

static uint16_t with_alpha(uint8_t r, uint8_t g, uint8_t b, float coverage)
{
    int a = (int)(coverage * ALPHA3_MAX + 0.5f);
    if (a >= ALPHA3_MAX) {
        return gc_rgb5a3_opaque(r, g, b);
    }
    return (uint16_t)((a << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4));
}

bool gc_disc_rgb5a3(uint8_t* out, size_t out_size, uint32_t dim, uint8_t r, uint8_t g, uint8_t b)
{
    if (out == NULL || !block_size(dim, dim) || out_size < gc_rgb5a3_size(dim, dim)) {
        return false;
    }
    float radius = dim / 2.0f;
    for (uint32_t y = 0; y < dim; y++) {
        for (uint32_t x = 0; x < dim; x++) {
            float dx = x + 0.5f - radius;
            float dy = y + 0.5f - radius;
            float coverage = (radius - sqrtf(dx * dx + dy * dy)) / DISC_EDGE;
            coverage = coverage < 0 ? 0 : coverage > 1 ? 1 : coverage;
            put(out, dim, x, y, with_alpha(r, g, b, coverage));
        }
    }
    return true;
}

bool gc_checker_rgb5a3(uint8_t* out, size_t out_size, uint32_t width, uint32_t height, uint32_t square,
                       uint16_t first, uint16_t second)
{
    if (out == NULL || !block_size(width, height) || square == 0 || out_size < gc_rgb5a3_size(width, height)) {
        return false;
    }
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            put(out, width, x, y, ((x / square) + (y / square)) % 2 == 0 ? first : second);
        }
    }
    return true;
}

bool gc_arrow_rgb5a3(uint8_t* out, size_t out_size, uint32_t dim, bool up, uint8_t r, uint8_t g, uint8_t b)
{
    if (out == NULL || !block_size(dim, dim) || out_size < gc_rgb5a3_size(dim, dim)) {
        return false;
    }
    float centre = dim / 2.0f;
    float tip = dim * ARROW_TIP;
    float base = dim * ARROW_BASE;
    float half = dim * ARROW_HALF_W;
    float height = base - tip;
    float side = sqrtf(half * half + height * height);
    for (uint32_t y = 0; y < dim; y++) {
        float py = (up ? y : dim - 1 - y) + 0.5f;
        for (uint32_t x = 0; x < dim; x++) {
            float half_here = half * (py - tip) / height;
            float to_side = (half_here - fabsf(x + 0.5f - centre)) * height / side;
            float to_base = base - py;
            float inside = to_side < to_base ? to_side : to_base;
            float coverage = inside / ARROW_EDGE;
            coverage = coverage < 0 ? 0 : coverage > 1 ? 1 : coverage;
            put(out, dim, x, y, with_alpha(r, g, b, coverage));
        }
    }
    return true;
}
