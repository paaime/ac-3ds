#include <stdio.h>

#include "../press_hold.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define HOLD 2000u

static void test_idle_does_nothing(void)
{
    PressStep s = press_hold_step(press_hold_idle(), true, 5000, HOLD);
    CHECK(s.event == PRESS_NONE && !s.state.active);
    s = press_hold_step(press_hold_idle(), false, 5000, HOLD);
    CHECK(s.event == PRESS_NONE);
}

static void test_a_short_press_is_a_tap_on_release(void)
{
    PressHold p = press_hold_begin(1000);
    PressStep s = press_hold_step(p, true, 1500, HOLD);
    CHECK(s.event == PRESS_NONE && s.state.active);
    s = press_hold_step(s.state, false, 1600, HOLD);
    CHECK(s.event == PRESS_TAP && !s.state.active);
    s = press_hold_step(s.state, false, 1700, HOLD);
    CHECK(s.event == PRESS_NONE);
}

static void test_holding_fires_once_and_the_release_is_silent(void)
{
    PressHold p = press_hold_begin(1000);
    PressStep s = press_hold_step(p, true, 2999, HOLD);
    CHECK(s.event == PRESS_NONE);
    s = press_hold_step(s.state, true, 3000, HOLD);
    CHECK(s.event == PRESS_HOLD && s.state.active);
    s = press_hold_step(s.state, true, 4000, HOLD);
    CHECK(s.event == PRESS_NONE && s.state.active);
    s = press_hold_step(s.state, false, 4100, HOLD);
    CHECK(s.event == PRESS_NONE && !s.state.active);
}

static void test_a_new_press_starts_over(void)
{
    PressStep s = press_hold_step(press_hold_begin(0), true, 2500, HOLD);
    CHECK(s.event == PRESS_HOLD);
    PressHold p = press_hold_begin(10000);
    CHECK(p.active && !p.fired);
    s = press_hold_step(p, false, 10100, HOLD);
    CHECK(s.event == PRESS_TAP);
}

static void test_a_clock_going_back_is_no_hold(void)
{
    PressStep s = press_hold_step(press_hold_begin(5000), true, 4000, HOLD);
    CHECK(s.event == PRESS_NONE && s.state.active);
}

int main(void)
{
    test_idle_does_nothing();
    test_a_short_press_is_a_tap_on_release();
    test_holding_fires_once_and_the_release_is_silent();
    test_a_new_press_starts_over();
    test_a_clock_going_back_is_no_hold();
    if (failures != 0) {
        fprintf(stderr, "press_hold: %d check(s) failed\n", failures);
        return 1;
    }
    printf("press_hold: all tests passed\n");
    return 0;
}
