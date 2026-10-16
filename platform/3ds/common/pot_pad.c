#include "pot_pad.h"

#include <stddef.h>

uint32_t pot_pad_side(uint32_t side)
{
    if (side == 0 || side > POT_PAD_MAX_SIDE) {
        return 0;
    }
    uint32_t padded = POT_PAD_MIN_SIDE;
    while (padded < side) {
        padded <<= 1;
    }
    return padded;
}

static bool is_power_of_two(uint32_t v)
{
    return v != 0 && (v & (v - 1)) == 0;
}

PotPadWrap pot_pad_effective_wrap(uint32_t side, PotPadWrap wrap)
{
    return pot_pad_side(side) == side ? POT_PAD_EDGE : wrap;
}

PotPadAxis pot_pad_axis(uint32_t side, PotPadWrap wrap)
{
    PotPadAxis a = { pot_pad_side(side), false, 1.0f };
    if (a.padded == 0) {
        return a;
    }
    a.stretch = pot_pad_effective_wrap(side, wrap) != POT_PAD_EDGE && !is_power_of_two(side);
    a.uv_scale = a.stretch ? 1.0f : (float)side / (float)a.padded;
    return a;
}

static uint32_t source_index(uint32_t x, uint32_t side, PotPadAxis axis, PotPadWrap wrap)
{
    if (axis.stretch) {
        return (uint32_t)(((uint64_t)x * side) / axis.padded);
    }
    if (x < side) {
        return x;
    }
    switch (wrap) {
    case POT_PAD_REPEAT:
        return x % side;
    case POT_PAD_MIRROR: {
        uint32_t m = x % (2 * side);
        return m < side ? m : 2 * side - 1 - m;
    }
    default:
        return side - 1;
    }
}

bool pot_pad_rgba8_wrap(uint32_t* dst, const uint32_t* src, uint32_t width, uint32_t height, PotPadWrap wrap_s,
                        PotPadWrap wrap_t)
{
    PotPadAxis s_axis = pot_pad_axis(width, wrap_s);
    PotPadAxis t_axis = pot_pad_axis(height, wrap_t);
    if (dst == NULL || src == NULL || s_axis.padded == 0 || t_axis.padded == 0) {
        return false;
    }
    uint16_t column[POT_PAD_MAX_SIDE];
    for (uint32_t x = 0; x < s_axis.padded; x++) {
        column[x] = (uint16_t)source_index(x, width, s_axis, wrap_s);
    }
    for (uint32_t y = 0; y < t_axis.padded; y++) {
        const uint32_t* row = &src[(size_t)source_index(y, height, t_axis, wrap_t) * width];
        uint32_t* out = &dst[(size_t)y * s_axis.padded];
        for (uint32_t x = 0; x < s_axis.padded; x++) {
            out[x] = row[column[x]];
        }
    }
    return true;
}

