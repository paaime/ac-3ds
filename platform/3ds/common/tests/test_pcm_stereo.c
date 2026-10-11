/* Host tests for pcm_stereo.c. */
#include "pcm_stereo.h"

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

static void test_each_sample_goes_to_both_sides(void)
{
    const int16_t src[3] = { 100, -200, 300 };
    int16_t dst[6];
    pcm_mono_to_stereo(dst, src, 3);
    CHECK(dst[0] == 100 && dst[1] == 100);
    CHECK(dst[2] == -200 && dst[3] == -200);
    CHECK(dst[4] == 300 && dst[5] == 300);
}

static void test_extremes_unchanged(void)
{
    const int16_t src[2] = { 32767, -32768 };
    int16_t dst[4];
    pcm_mono_to_stereo(dst, src, 2);
    CHECK(dst[0] == 32767 && dst[1] == 32767 && dst[2] == -32768 && dst[3] == -32768);
}

static void test_nothing_in_nothing_written(void)
{
    const int16_t src[1] = { 5 };
    int16_t dst[2] = { 7, 7 };
    pcm_mono_to_stereo(dst, src, 0);
    CHECK(dst[0] == 7 && dst[1] == 7);
}

int main(void)
{
    test_each_sample_goes_to_both_sides();
    test_extremes_unchanged();
    test_nothing_in_nothing_written();
    if (failures == 0) {
        printf("pcm_stereo: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
