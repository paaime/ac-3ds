#ifndef AC3DS_PAD_MAP_H
#define AC3DS_PAD_MAP_H

#include <stdint.h>

/* libctru KEY_* bits */
enum {
    N3DS_KEY_A = 1u << 0,
    N3DS_KEY_B = 1u << 1,
    N3DS_KEY_SELECT = 1u << 2,
    N3DS_KEY_START = 1u << 3,
    N3DS_KEY_DRIGHT = 1u << 4,
    N3DS_KEY_DLEFT = 1u << 5,
    N3DS_KEY_DUP = 1u << 6,
    N3DS_KEY_DDOWN = 1u << 7,
    N3DS_KEY_R = 1u << 8,
    N3DS_KEY_L = 1u << 9,
    N3DS_KEY_X = 1u << 10,
    N3DS_KEY_Y = 1u << 11,
    N3DS_KEY_ZL = 1u << 14,
    N3DS_KEY_ZR = 1u << 15,
};

/* dolphin/pad.h PAD_BUTTON_* / PAD_TRIGGER_* bits */
enum {
    GC_PAD_LEFT = 1u << 0,
    GC_PAD_RIGHT = 1u << 1,
    GC_PAD_DOWN = 1u << 2,
    GC_PAD_UP = 1u << 3,
    GC_PAD_Z = 1u << 4,
    GC_PAD_R = 1u << 5,
    GC_PAD_L = 1u << 6,
    GC_PAD_A = 1u << 8,
    GC_PAD_B = 1u << 9,
    GC_PAD_X = 1u << 10,
    GC_PAD_Y = 1u << 11,
    GC_PAD_START = 1u << 12,
};

#define PAD_MAP_STICK_RANGE     156 /* calibrated Circle Pad / C-Stick full tilt */
#define PAD_MAP_STICK_DEAD_ZONE 18  /* PADClamp's 15 GC units (Padclamp.c) in Circle Pad units */

typedef struct {
    uint32_t keys;
    int16_t circle_x, circle_y;
    int16_t cstick_x, cstick_y;
} N3dsInput;

typedef struct {
    uint16_t button;
    int8_t stick_x, stick_y;
    int8_t substick_x, substick_y;
    uint8_t trigger_l, trigger_r;
} GcPadState;

GcPadState pad_map(const N3dsInput* in);

#endif /* AC3DS_PAD_MAP_H */
