#include "pad_map.h"

#include <math.h>

#define STICK_MAX      127
#define TRIGGER_FULL   255

typedef struct {
    int8_t x, y;
} StickXY;

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

static StickXY scale_stick(int16_t x, int16_t y)
{
    StickXY s = { 0, 0 };
    float r = sqrtf((float)x * x + (float)y * y);
    if (r <= PAD_MAP_STICK_DEAD_ZONE) {
        return s;
    }
    float tilt = r < PAD_MAP_STICK_RANGE ? r : PAD_MAP_STICK_RANGE;
    float k = (tilt - PAD_MAP_STICK_DEAD_ZONE) * STICK_MAX / ((PAD_MAP_STICK_RANGE - PAD_MAP_STICK_DEAD_ZONE) * r);
    s.x = (int8_t)lroundf(x * k);
    s.y = (int8_t)lroundf(y * k);
    return s;
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
    StickXY circle = scale_stick(in->circle_x, in->circle_y);
    StickXY cstick = scale_stick(in->cstick_x, in->cstick_y);
    s.stick_x = circle.x;
    s.stick_y = circle.y;
    s.substick_x = cstick.x;
    s.substick_y = cstick.y;
    return s;
}
