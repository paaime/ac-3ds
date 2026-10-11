/* Host tests for gc_clock.c. */
#include "gc_clock.h"

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

#define N3DS_ARM11_HZ 268111856ull /* SYSCLOCK_ARM11 */
#define GC_TIMER_HZ   40500000ull  /* GC bus clock / 4 */
#define SECONDS_30D   (30ull * 24 * 3600)

static void test_small_values_match_naive(void)
{
    uint64_t first_bad = UINT64_MAX;
    for (uint64_t t = 0; t < 2000000ull && first_bad == UINT64_MAX; t += 9973) {
        if (gc_clock_convert(t, N3DS_ARM11_HZ, GC_TIMER_HZ) != t * GC_TIMER_HZ / N3DS_ARM11_HZ) {
            first_bad = t;
        }
    }
    CHECK(first_bad == UINT64_MAX);
}

static void test_whole_seconds(void)
{
    CHECK(gc_clock_convert(N3DS_ARM11_HZ, N3DS_ARM11_HZ, GC_TIMER_HZ) == GC_TIMER_HZ);
    CHECK(gc_clock_convert(60 * N3DS_ARM11_HZ, N3DS_ARM11_HZ, GC_TIMER_HZ) == 60 * GC_TIMER_HZ);
}

static void test_no_overflow_after_the_naive_limit(void)
{
    /* t * 40.5e6 overflows u64 after ~28 minutes of 3DS ticks. */
    uint64_t t = SECONDS_30D * N3DS_ARM11_HZ + 12345;
    uint64_t got = gc_clock_convert(t, N3DS_ARM11_HZ, GC_TIMER_HZ);
    uint64_t want = SECONDS_30D * GC_TIMER_HZ + 12345 * GC_TIMER_HZ / N3DS_ARM11_HZ;
    CHECK(got == want);
}

static void test_monotonic_across_a_second_boundary(void)
{
    uint64_t prev = 0;
    for (uint64_t t = N3DS_ARM11_HZ - 50; t < N3DS_ARM11_HZ + 50; t++) {
        uint64_t v = gc_clock_convert(t, N3DS_ARM11_HZ, GC_TIMER_HZ);
        CHECK(v >= prev);
        prev = v;
    }
}

static void test_zero_source_rate(void)
{
    CHECK(gc_clock_convert(1000, 0, GC_TIMER_HZ) == 0);
}

/* The system tick stops while the console sleeps; the wall clock doesn't. */
#define THRESHOLD_MS 2000

static void test_in_step_no_catch_up(void)
{
    CHECK(gc_clock_catch_up_ms(60000, 60000, THRESHOLD_MS) == 0);
    CHECK(gc_clock_catch_up_ms(61999, 60000, THRESHOLD_MS) == 0); /* drift, not sleep */
    CHECK(gc_clock_catch_up_ms(62000, 60000, THRESHOLD_MS) == 0);
}

static void test_after_sleep_the_gap_is_made_up(void)
{
    CHECK(gc_clock_catch_up_ms(60000 + 600000, 60000, THRESHOLD_MS) == 600000); /* ten minutes asleep */
    CHECK(gc_clock_catch_up_ms(62001, 60000, THRESHOLD_MS) == 2001);
}

static void test_never_backwards(void)
{
    CHECK(gc_clock_catch_up_ms(60000, 90000, THRESHOLD_MS) == 0); /* the clock ahead of the wall */
    CHECK(gc_clock_catch_up_ms(-5000, 0, THRESHOLD_MS) == 0);      /* the wall clock set back */
}

int main(void)
{
    test_small_values_match_naive();
    test_whole_seconds();
    test_no_overflow_after_the_naive_limit();
    test_monotonic_across_a_second_boundary();
    test_zero_source_rate();
    test_in_step_no_catch_up();
    test_after_sleep_the_gap_is_made_up();
    test_never_backwards();
    if (failures != 0) {
        fprintf(stderr, "gc_clock: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gc_clock: all tests passed\n");
    return 0;
}
