#include "pad_map.h"

#define STICK_MAX      127
#define STICK_MIN      (-128)
#define TRIGGER_FULL   255

static const struct {
    uint32_t key;
    uint16_t button;
} BUTTONS[] = {
    { N3DS_KEY_A, GC_PAD_A },         { N3DS_KEY_B, GC_PAD_B },          { N3DS_KEY_X, GC_PAD_X },
    { N3DS_KEY_Y, GC_PAD_Y },         { N3DS_KEY_START, GC_PAD_START },  { N3DS_KEY_L, GC_PAD_L },
    { N3DS_KEY_R, GC_PAD_R },         { N3DS_KEY_ZL, GC_PAD_Z },         { N3DS_KEY_ZR, GC_PAD_Z },
    { N3DS_KEY_DUP, GC_PAD_UP },      { N3DS_KEY_DDOWN, GC_PAD_DOWN },   { N3DS_KEY_DLEFT, GC_PAD_LEFT },
    { N3DS_KEY_DRIGHT, GC_PAD_RIGHT },
};

static int8_t scale_axis(int16_t v)
{
    int scaled = (int)v * STICK_MAX / PAD_MAP_STICK_RANGE;
    if (scaled > STICK_MAX) {
        return STICK_MAX;
    }
    return (int8_t)(scaled < STICK_MIN ? STICK_MIN : scaled);
}

GcPadState pad_map(const N3dsInput* in)
{
    GcPadState s = { 0, 0, 0, 0, 0, 0, 0 };
    for (unsigned i = 0; i < sizeof(BUTTONS) / sizeof(BUTTONS[0]); i++) {
        if (in->keys & BUTTONS[i].key) {
            s.button |= BUTTONS[i].button;
        }
    }
    s.trigger_l = (in->keys & N3DS_KEY_L) ? TRIGGER_FULL : 0;
    s.trigger_r = (in->keys & N3DS_KEY_R) ? TRIGGER_FULL : 0;
    s.stick_x = scale_axis(in->circle_x);
    s.stick_y = scale_axis(in->circle_y);
    s.substick_x = scale_axis(in->cstick_x);
    s.substick_y = scale_axis(in->cstick_y);
    return s;
}
