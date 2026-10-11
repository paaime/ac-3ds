/* Host tests for pcm_level.c. */
#include "pcm_level.h"

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

static void test_silence_is_zero(void)
{
    const int16_t s[4] = { 0, 0, 0, 0 };
    CHECK(pcm_peak(s, 4) == 0);
    CHECK(pcm_peak(s, 0) == 0);
}

static void test_peak_is_the_largest_magnitude(void)
{
    const int16_t s[5] = { 10, -300, 299, 5, -2 };
    CHECK(pcm_peak(s, 5) == 300);
}

static void test_most_negative_sample_does_not_overflow(void)
{
    const int16_t s[2] = { 32767, -32768 };
    CHECK(pcm_peak(s, 2) == 32768);
}

static void test_percent_of_full_scale(void)
{
    CHECK(pcm_peak_percent(0) == 0);
    CHECK(pcm_peak_percent(32768) == 100);
    CHECK(pcm_peak_percent(16384) == 50);
    CHECK(pcm_peak_percent(1) == 1); /* any signal shows */
}

int main(void)
{
    test_silence_is_zero();
    test_peak_is_the_largest_magnitude();
    test_most_negative_sample_does_not_overflow();
    test_percent_of_full_scale();
    if (failures == 0) {
        printf("pcm_level: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
