/* Host tests for light_timer.c. */
#include "light_timer.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

#define TIMEOUT 10000u

static void test_starts_lit(void)
{
    LightTimer t = light_timer_start(500);
    CHECK(t.lit);
    CHECK(t.last_touch_ms == 500);
}

static void test_without_the_saver_stays_lit(void)
{
    LightTimer t = light_timer_start(0);
    t = light_timer_step(t, false, false, 999999, TIMEOUT);
    CHECK(t.lit);
}

static void test_goes_dark_after_the_timeout(void)
{
    LightTimer t = light_timer_start(0);
    t = light_timer_step(t, true, false, TIMEOUT - 1, TIMEOUT);
    CHECK(t.lit);
    t = light_timer_step(t, true, false, TIMEOUT, TIMEOUT);
    CHECK(!t.lit);
}

static void test_a_touch_lights_it_again_and_restarts_the_wait(void)
{
    LightTimer t = light_timer_start(0);
    t = light_timer_step(t, true, false, TIMEOUT + 5, TIMEOUT);
    CHECK(!t.lit);
    t = light_timer_step(t, true, true, TIMEOUT + 10, TIMEOUT);
    CHECK(t.lit && t.last_touch_ms == TIMEOUT + 10);
    t = light_timer_step(t, true, false, 2 * TIMEOUT + 9, TIMEOUT);
    CHECK(t.lit);
    t = light_timer_step(t, true, false, 2 * TIMEOUT + 10, TIMEOUT);
    CHECK(!t.lit);
}

static void test_turning_the_saver_off_lights_it(void)
{
    LightTimer t = light_timer_step(light_timer_start(0), true, false, TIMEOUT, TIMEOUT);
    CHECK(!t.lit);
    t = light_timer_step(t, false, false, TIMEOUT + 1, TIMEOUT);
    CHECK(t.lit);
    /* and the wait starts from then, not from the old touch */
    t = light_timer_step(t, true, false, TIMEOUT + 2, TIMEOUT);
    CHECK(t.lit);
}

static void test_the_step_returns_a_copy(void)
{
    LightTimer t = light_timer_start(0);
    LightTimer dark = light_timer_step(t, true, false, TIMEOUT, TIMEOUT);
    CHECK(t.lit && !dark.lit);
}

static void test_a_clock_going_back_does_not_darken(void)
{
    LightTimer t = light_timer_start(5000);
    t = light_timer_step(t, true, false, 100, TIMEOUT); /* now before the last touch */
    CHECK(t.lit);
}

int main(void)
{
    test_starts_lit();
    test_without_the_saver_stays_lit();
    test_goes_dark_after_the_timeout();
    test_a_touch_lights_it_again_and_restarts_the_wait();
    test_turning_the_saver_off_lights_it();
    test_the_step_returns_a_copy();
    test_a_clock_going_back_does_not_darken();
    if (failures != 0) {
        fprintf(stderr, "light_timer: %d check(s) failed\n", failures);
        return 1;
    }
    printf("light_timer: all tests passed\n");
    return 0;
}
