/* Host tests for feed_rate.c. */
#include "feed_rate.h"

#include <math.h>
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

#define HZ      1000000u /* ticks per second */
#define NOMINAL 27965.0f

static FeedRate feed(FeedRate r, uint32_t frames_per_call, uint32_t calls_per_second, int seconds)
{
    uint64_t step = HZ / calls_per_second;
    for (uint32_t i = 0; i < calls_per_second * (uint32_t)seconds; i++) {
        r = feed_rate_add(r, frames_per_call, r.last_tick + step);
    }
    return r;
}

static void test_starts_at_nominal(void)
{
    FeedRate r = feed_rate_create(NOMINAL, HZ);
    CHECK(feed_rate_value(r) == NOMINAL);
}

static void test_follows_the_measured_rate(void)
{
    FeedRate r = feed_rate_create(NOMINAL, HZ);
    r = feed(r, 465, 60, 3); /* 27,900 frames per second */
    CHECK(fabsf(feed_rate_value(r) - 27900.0f) < 50.0f);
}

static void test_slow_feed_slows_down_but_is_clamped(void)
{
    FeedRate r = feed_rate_create(NOMINAL, HZ);
    r = feed(r, 465, 15, 3); /* a quarter speed */
    CHECK(fabsf(feed_rate_value(r) - NOMINAL * FEED_RATE_MIN_RATIO) < 1.0f);
    r = feed(r, 465 * 8, 60, 3); /* far too fast */
    CHECK(fabsf(feed_rate_value(r) - NOMINAL * FEED_RATE_MAX_RATIO) < 1.0f);
}

static void test_a_stall_restarts_the_window(void)
{
    FeedRate r = feed_rate_create(NOMINAL, HZ);
    r = feed(r, 465, 60, 2);
    float before = feed_rate_value(r);
    r = feed_rate_add(r, 465, r.last_tick + 5ull * HZ); /* 5 s pause (loading, HOME menu) */
    CHECK(feed_rate_value(r) == before);
}

static void test_first_call_only_starts_the_clock(void)
{
    FeedRate r = feed_rate_create(NOMINAL, HZ);
    r = feed_rate_add(r, 100000, 12345);
    CHECK(feed_rate_value(r) == NOMINAL);
}

int main(void)
{
    test_starts_at_nominal();
    test_follows_the_measured_rate();
    test_slow_feed_slows_down_but_is_clamped();
    test_a_stall_restarts_the_window();
    test_first_call_only_starts_the_clock();
    if (failures == 0) {
        printf("feed_rate: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
