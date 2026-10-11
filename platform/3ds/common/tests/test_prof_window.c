/* Host tests for prof_window.c. */
#include "prof_window.h"

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

#define NEAR_EQ(a, b) (fabs((a) - (b)) < 1e-6)
#define HZ 1000000u /* 1 tick = 1 us */

static void test_empty_window_reads_zero(void)
{
    ProfWindow w = prof_window_create();
    CHECK(prof_window_frames(w) == 0);
    CHECK(prof_window_ms(w, 0, HZ) == 0.0);
    CHECK(prof_window_frame_ms(w, HZ) == 0.0);
}

static void test_averages_per_frame(void)
{
    ProfWindow w = prof_window_create();
    w = prof_window_add(w, 2, 4000);  /* 4 ms in slot 2 */
    w = prof_window_end_frame(w, 16000);
    w = prof_window_add(w, 2, 2000);
    w = prof_window_add(w, 2, 2000);
    w = prof_window_end_frame(w, 18000);
    CHECK(prof_window_frames(w) == 2);
    CHECK(NEAR_EQ(prof_window_ms(w, 2, HZ), 4.0));
    CHECK(NEAR_EQ(prof_window_frame_ms(w, HZ), 17.0));
    CHECK(prof_window_ms(w, 3, HZ) == 0.0);
}

static void test_out_of_range_slot_is_ignored(void)
{
    ProfWindow w = prof_window_create();
    w = prof_window_add(w, PROF_WINDOW_SLOTS, 5000);
    w = prof_window_end_frame(w, 1000);
    CHECK(prof_window_ms(w, PROF_WINDOW_SLOTS, HZ) == 0.0);
    for (unsigned i = 0; i < PROF_WINDOW_SLOTS; i++) {
        CHECK(prof_window_ms(w, i, HZ) == 0.0);
    }
}

static void test_inputs_are_not_modified(void)
{
    ProfWindow a = prof_window_create();
    ProfWindow b = prof_window_add(a, 0, 100);
    CHECK(prof_window_end_frame(a, 10).frames == 1);
    CHECK(a.ticks[0] == 0 && b.ticks[0] == 100);
}

static void test_share_of_a_period(void)
{
    CHECK(NEAR_EQ(prof_share_percent(250, 1000), 25.0));
    CHECK(prof_share_percent(5, 0) == 0.0);
}

int main(void)
{
    test_empty_window_reads_zero();
    test_averages_per_frame();
    test_out_of_range_slot_is_ignored();
    test_inputs_are_not_modified();
    test_share_of_a_period();
    if (failures == 0) {
        printf("prof_window: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
