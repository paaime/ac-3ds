#ifndef AC3DS_POT_PAD_H
#define AC3DS_POT_PAD_H

/* Power-of-two padding for PICA200 textures (8 to 1024 per side). Texture
 * coordinates are scaled by side / padded side, except on a stretched axis. */

#include <stdbool.h>
#include <stdint.h>

#define POT_PAD_MIN_SIDE 8
#define POT_PAD_MAX_SIDE 1024

/* 0 if side is 0 or the result would exceed POT_PAD_MAX_SIDE. */
uint32_t pot_pad_side(uint32_t side);

/* GXTexWrapMode values. EDGE repeats the last texel; REPEAT and MIRROR fill the
 * padding with copies of a power-of-two side, or stretch a side that isn't one
 * over the padded size (nearest). */
typedef enum {
    POT_PAD_EDGE = 0,
    POT_PAD_REPEAT = 1,
    POT_PAD_MIRROR = 2,
} PotPadWrap;

typedef struct {
    uint32_t padded; /* 0 if the side is invalid */
    bool stretch;
    float uv_scale;
} PotPadAxis;

PotPadAxis pot_pad_axis(uint32_t side, PotPadWrap wrap);

/* EDGE when the side needs no padding, else `wrap`: the texture cache's key. */
PotPadWrap pot_pad_effective_wrap(uint32_t side, PotPadWrap wrap);

/* dst is pot_pad_side(width) x pot_pad_side(height) and must not overlap src.
 * False (dst untouched) on invalid sizes or NULL pointers. */
bool pot_pad_rgba8_wrap(uint32_t* dst, const uint32_t* src, uint32_t width, uint32_t height, PotPadWrap wrap_s,
                        PotPadWrap wrap_t);

#endif /* AC3DS_POT_PAD_H */
