/* Host tests for pad_map.c. */
#include "pad_map.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static N3dsInput keys(uint32_t held)
{
    N3dsInput in;
    memset(&in, 0, sizeof(in));
    in.keys = held;
    return in;
}

static void test_face_buttons_and_dpad(void)
{
    static const struct {
        uint32_t key;
        uint16_t button;
    } MAP[] = {
        { N3DS_KEY_A, GC_PAD_A },          { N3DS_KEY_B, GC_PAD_B },
        { N3DS_KEY_X, GC_PAD_X },          { N3DS_KEY_Y, GC_PAD_Y },
        { N3DS_KEY_START, GC_PAD_START },  { N3DS_KEY_DUP, GC_PAD_UP },
        { N3DS_KEY_DDOWN, GC_PAD_DOWN },   { N3DS_KEY_DLEFT, GC_PAD_LEFT },
        { N3DS_KEY_DRIGHT, GC_PAD_RIGHT }, { N3DS_KEY_ZL, GC_PAD_Z },
        { N3DS_KEY_ZR, GC_PAD_Z },
    };
    for (size_t i = 0; i < sizeof(MAP) / sizeof(MAP[0]); i++) {
        N3dsInput in = keys(MAP[i].key);
        GcPadState s = pad_map(&in);
        CHECK(s.button == MAP[i].button);
        CHECK(s.trigger_l == 0 && s.trigger_r == 0);
    }
}

static void test_shoulders_are_full_presses(void)
{
    N3dsInput in = keys(N3DS_KEY_L | N3DS_KEY_R);
    GcPadState s = pad_map(&in);
    CHECK(s.button == (GC_PAD_L | GC_PAD_R));
    CHECK(s.trigger_l == 255 && s.trigger_r == 255);
}

static void test_select_is_left_unmapped(void)
{
    N3dsInput in = keys(N3DS_KEY_SELECT);
    CHECK(pad_map(&in).button == 0);
}

static void test_sticks_scale_and_clamp(void)
{
    N3dsInput in = keys(0);
    GcPadState s = pad_map(&in);
    CHECK(s.stick_x == 0 && s.stick_y == 0 && s.substick_x == 0 && s.substick_y == 0);

    in.circle_x = PAD_MAP_STICK_RANGE;
    in.circle_y = -PAD_MAP_STICK_RANGE;
    in.cstick_x = -PAD_MAP_STICK_RANGE / 2;
    in.cstick_y = PAD_MAP_STICK_RANGE / 2;
    s = pad_map(&in);
    CHECK(s.stick_x == 127 && s.stick_y == -127);
    CHECK(s.substick_x == -63 && s.substick_y == 63);

    in.circle_x = 400; /* beyond the range: clamped */
    in.circle_y = -400;
    s = pad_map(&in);
    CHECK(s.stick_x == 127 && s.stick_y == -128);
}

int main(void)
{
    test_face_buttons_and_dpad();
    test_shoulders_are_full_presses();
    test_select_is_left_unmapped();
    test_sticks_scale_and_clamp();
    if (failures != 0) {
        fprintf(stderr, "pad_map: %d check(s) failed\n", failures);
        return 1;
    }
    printf("pad_map: all tests passed\n");
    return 0;
}
