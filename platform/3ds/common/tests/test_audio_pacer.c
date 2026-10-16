#include "audio_pacer.h"

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

#define RATE  32000
#define CHUNK 560 /* stereo frames one mixer call produces */

static AudioPacer catch_up(AudioPacer p, uint64_t due, int* calls)
{
    p = audio_pacer_advance_to(p, due);
    *calls = 0;
    while (audio_pacer_behind(p) && *calls < AUDIO_PACER_MAX_MIXES) {
        p = audio_pacer_produced(p, CHUNK);
        (*calls)++;
    }
    return p;
}

static void test_starts_caught_up(void)
{
    CHECK(!audio_pacer_behind(audio_pacer_create(RATE)));
}

static void test_follows_the_clock_not_the_frame_rate(void)
{
    AudioPacer p = audio_pacer_create(RATE);
    long total = 0;
    for (int f = 1; f <= 30; f++) {
        int calls;
        p = catch_up(p, (uint64_t)RATE * f / 30, &calls);
        total += calls;
    }
    CHECK(total == (RATE + CHUNK - 1) / CHUNK);
}

static void test_resyncs_after_a_long_stall(void)
{
    AudioPacer p = audio_pacer_create(RATE);
    int calls;
    p = catch_up(p, 5ull * RATE, &calls);
    CHECK(calls <= AUDIO_PACER_MAX_MIXES);
    CHECK(p.produced + RATE > p.due);
    p = catch_up(p, 5ull * RATE + RATE / 60, &calls);
    CHECK(calls <= 1);
}

static void test_never_far_ahead(void)
{
    AudioPacer p = audio_pacer_create(RATE);
    int calls;
    p = catch_up(p, RATE / 60, &calls);
    CHECK(calls == 1);
    p = catch_up(p, 2 * RATE / 60, &calls);
    CHECK(calls == 1);
}

static void test_clock_never_goes_back(void)
{
    AudioPacer p = audio_pacer_create(RATE);
    p = audio_pacer_advance_to(p, 1000);
    p = audio_pacer_advance_to(p, 500);
    CHECK(p.due == 1000);
}

int main(void)
{
    test_starts_caught_up();
    test_follows_the_clock_not_the_frame_rate();
    test_resyncs_after_a_long_stall();
    test_never_far_ahead();
    test_clock_never_goes_back();
    if (failures != 0) {
        fprintf(stderr, "audio_pacer: %d check(s) failed\n", failures);
        return 1;
    }
    printf("audio_pacer: all tests passed\n");
    return 0;
}
