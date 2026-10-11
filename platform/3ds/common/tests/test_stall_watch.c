/* Host tests for stall_watch.c. */
#include "stall_watch.h"

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

#define LIMIT  4000u
#define REPEAT 15000u

static void test_moving_counter_reports_nothing(void)
{
    StallWatch w = stall_watch_start(0, 0);
    for (uint32_t t = 1; t < 100; t++) {
        w = stall_watch_check(w, t, t * 1000u, LIMIT, REPEAT);
        CHECK(w.event == STALL_NONE);
    }
}

static void test_a_stall_is_reported_once_at_the_limit(void)
{
    StallWatch w = stall_watch_start(7, 1000);
    w = stall_watch_check(w, 7, 1000 + LIMIT - 1, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE);
    w = stall_watch_check(w, 7, 1000 + LIMIT, LIMIT, REPEAT);
    CHECK(w.event == STALL_BEGAN);
    w = stall_watch_check(w, 7, 1000 + LIMIT + 500, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE);
}

static void test_a_long_stall_is_reported_again(void)
{
    StallWatch w = stall_watch_start(7, 0);
    w = stall_watch_check(w, 7, LIMIT, LIMIT, REPEAT);
    CHECK(w.event == STALL_BEGAN);
    w = stall_watch_check(w, 7, LIMIT + REPEAT - 1, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE);
    w = stall_watch_check(w, 7, LIMIT + REPEAT, LIMIT, REPEAT);
    CHECK(w.event == STALL_GOES_ON);
    w = stall_watch_check(w, 7, LIMIT + 2 * REPEAT, LIMIT, REPEAT);
    CHECK(w.event == STALL_GOES_ON);
}

static void test_moving_again_ends_a_reported_stall_only(void)
{
    StallWatch w = stall_watch_start(7, 0);
    w = stall_watch_check(w, 8, 100, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE); /* no stall was reported */
    w = stall_watch_check(w, 8, 100 + LIMIT, LIMIT, REPEAT);
    CHECK(w.event == STALL_BEGAN);
    w = stall_watch_check(w, 9, 100 + LIMIT + 10, LIMIT, REPEAT);
    CHECK(w.event == STALL_ENDED);
    CHECK(w.since_ms == 100 + LIMIT + 10);
    w = stall_watch_check(w, 9, 100 + LIMIT + 11, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE);
    w = stall_watch_check(w, 9, 100 + 2 * LIMIT + 10, LIMIT, REPEAT);
    CHECK(w.event == STALL_BEGAN); /* a new stall */
}

static void test_the_check_returns_a_copy(void)
{
    StallWatch w = stall_watch_start(1, 0);
    StallWatch later = stall_watch_check(w, 1, LIMIT, LIMIT, REPEAT);
    CHECK(!w.reported && later.reported);
}

static void test_a_clock_going_back_is_not_a_stall(void)
{
    StallWatch w = stall_watch_start(1, 50000);
    w = stall_watch_check(w, 1, 10, LIMIT, REPEAT);
    CHECK(w.event == STALL_NONE);
}

int main(void)
{
    test_moving_counter_reports_nothing();
    test_a_stall_is_reported_once_at_the_limit();
    test_a_long_stall_is_reported_again();
    test_moving_again_ends_a_reported_stall_only();
    test_the_check_returns_a_copy();
    test_a_clock_going_back_is_not_a_stall();
    if (failures != 0) {
        fprintf(stderr, "stall_watch: %d check(s) failed\n", failures);
        return 1;
    }
    printf("stall_watch: all tests passed\n");
    return 0;
}
