#ifndef AC3DS_POT_PAD_H
#define AC3DS_POT_PAD_H

#include <stdbool.h>
#include <stdint.h>

#define POT_PAD_MIN_SIDE 8
#define POT_PAD_MAX_SIDE 1024

uint32_t pot_pad_side(uint32_t side);

/* GXTexWrapMode values; REPEAT and MIRROR stretch a non-power-of-two side over the padded size. */
typedef enum {
    POT_PAD_EDGE = 0,
    POT_PAD_REPEAT = 1,
    POT_PAD_MIRROR = 2,
} PotPadWrap;

typedef struct {
    uint32_t padded;
    bool stretch;
    float uv_scale;
} PotPadAxis;

PotPadAxis pot_pad_axis(uint32_t side, PotPadWrap wrap);

PotPadWrap pot_pad_effective_wrap(uint32_t side, PotPadWrap wrap);

/* dst is pot_pad_side(width) x pot_pad_side(height) and must not overlap src. */
bool pot_pad_rgba8_wrap(uint32_t* dst, const uint32_t* src, uint32_t width, uint32_t height, PotPadWrap wrap_s,
                        PotPadWrap wrap_t);

#endif /* AC3DS_POT_PAD_H */
