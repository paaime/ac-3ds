#include "look.h"

#define SAVE_TAG      0xC0u
#define SAVE_TAG_MASK 0xF0u
#define SAVE_VALUE    0x0Fu

static int clamp_tone(int tone)
{
    return tone < 0 ? 0 : tone >= LOOK_TONES ? LOOK_TONES - 1 : tone;
}

uint8_t look_floor_to_save(int tone)
{
    int t = clamp_tone(tone);
    return t == 0 ? 0 : (uint8_t)(SAVE_TAG | (unsigned)t);
}

int look_floor_from_save(uint8_t b)
{
    int tone = (int)(b & SAVE_VALUE);
    return (b & SAVE_TAG_MASK) == SAVE_TAG && tone < LOOK_TONES ? tone : 0;
}

int look_face_step(int face, int delta)
{
    int f = (face + delta) % LOOK_FACES;
    return f < 0 ? f + LOOK_FACES : f;
}

int look_tone_step(int tone, int delta)
{
    return clamp_tone(clamp_tone(tone) + delta);
}
