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

static GcPadState circle(int16_t x, int16_t y)
{
    N3dsInput in = keys(0);
    in.circle_x = x;
    in.circle_y = y;
    return pad_map(&in);
}

static GcPadState cstick(int16_t x, int16_t y)
{
    N3dsInput in = keys(0);
    in.cstick_x = x;
    in.cstick_y = y;
    return pad_map(&in);
}

static void test_sticks_at_rest_read_centred(void)
{
    GcPadState s = circle(0, 0);
    CHECK(s.stick_x == 0 && s.stick_y == 0 && s.substick_x == 0 && s.substick_y == 0);

    s = circle(12, -12);
    CHECK(s.stick_x == 0 && s.stick_y == 0);
    s = circle(PAD_MAP_STICK_DEAD_ZONE, 0);
    CHECK(s.stick_x == 0 && s.stick_y == 0);
    s = circle(0, -PAD_MAP_STICK_DEAD_ZONE);
    CHECK(s.stick_x == 0 && s.stick_y == 0);
}

static void test_sticks_rise_from_the_dead_zone(void)
{
    GcPadState s = circle(PAD_MAP_STICK_DEAD_ZONE + 1, 0);
    CHECK(s.stick_x == 1 && s.stick_y == 0);
    s = circle(30, 0); /* 12 * 127 / 138 = 11.04 */
    CHECK(s.stick_x == 11 && s.stick_y == 0);
    s = circle(0, -100); /* 82 * 127 / 138 = 75.46 */
    CHECK(s.stick_x == 0 && s.stick_y == -75);
}

static void test_sticks_reach_full_tilt(void)
{
    GcPadState s = circle(PAD_MAP_STICK_RANGE, 0);
    CHECK(s.stick_x == 127 && s.stick_y == 0);
    s = circle(0, -PAD_MAP_STICK_RANGE);
    CHECK(s.stick_x == 0 && s.stick_y == -127);

    s = circle(400, 0);
    CHECK(s.stick_x == 127 && s.stick_y == 0);
    s = circle(400, -400); /* 127 / sqrt(2) = 89.8 */
    CHECK(s.stick_x == 90 && s.stick_y == -90);
}

static void test_cstick_shares_the_dead_zone(void)
{
    GcPadState s = cstick(12, 12);
    CHECK(s.substick_x == 0 && s.substick_y == 0);
    s = cstick(-PAD_MAP_STICK_RANGE, 0);
    CHECK(s.substick_x == -127 && s.substick_y == 0);
    CHECK(s.stick_x == 0 && s.stick_y == 0);
}

int main(void)
{
    test_face_buttons_and_dpad();
    test_shoulders_are_full_presses();
    test_select_is_left_unmapped();
    test_sticks_at_rest_read_centred();
    test_sticks_rise_from_the_dead_zone();
    test_sticks_reach_full_tilt();
    test_cstick_shares_the_dead_zone();
    if (failures != 0) {
        fprintf(stderr, "pad_map: %d check(s) failed\n", failures);
        return 1;
    }
    printf("pad_map: all tests passed\n");
    return 0;
}
