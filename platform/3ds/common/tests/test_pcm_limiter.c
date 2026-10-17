#include "pcm_limiter.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

#define RATE       32028.5f
#define GAIN       1.5f
#define CEILING    29204
#define PERIOD     128
#define FULL_SCALE 32767.0f
#define PI         3.14159265358979f

static PcmLimiter limiter(void)
{
    return pcm_limiter_create(GAIN, (float)CEILING, RATE);
}

static int16_t tone(uint32_t i, float amplitude)
{
    return (int16_t)lrintf(amplitude * sinf(2.0f * PI * (float)(i % PERIOD) / PERIOD));
}

static int16_t* stereo_tone(uint32_t frames, float left, float right)
{
    int16_t* s = malloc((size_t)frames * 2 * sizeof(int16_t));
    for (uint32_t i = 0; i < frames; i++) {
        s[2 * i] = tone(i, left);
        s[2 * i + 1] = tone(i, right);
    }
    return s;
}

static int peak(const int16_t* s, uint32_t count)
{
    int p = 0;
    for (uint32_t i = 0; i < count; i++) {
        int m = abs(s[i]);
        p = m > p ? m : p;
    }
    return p;
}

static void test_quiet_sound_gets_the_whole_gain(void)
{
    const int16_t in[8] = { 1000, -2000, 0, 2, 10000, -10000, 19000, -19400 };
    const int16_t want[8] = { 1500, -3000, 0, 3, 15000, -15000, 28500, -29100 };
    int16_t out[8];
    pcm_limiter_run(limiter(), out, in, 4);
    CHECK(memcmp(out, want, sizeof(want)) == 0);
}

static void test_loud_sound_stays_under_the_ceiling(void)
{
    const uint32_t frames = 16000;
    int16_t* in = stereo_tone(frames, FULL_SCALE, -FULL_SCALE);
    int16_t* out = malloc((size_t)frames * 2 * sizeof(int16_t));
    pcm_limiter_run(limiter(), out, in, frames);
    CHECK(peak(out, frames * 2) <= CEILING);
    CHECK(peak(out, frames * 2) >= CEILING - 1);
    free(in);
    free(out);
}

static void test_a_steady_loud_tone_keeps_its_shape(void)
{
    const uint32_t frames = 8000;
    const float amplitude = 30000.0f;
    int16_t* in = stereo_tone(frames, amplitude, amplitude);
    int16_t* out = malloc((size_t)frames * 2 * sizeof(int16_t));
    pcm_limiter_run(limiter(), out, in, frames);
    float k = GAIN * CEILING / (amplitude * GAIN);
    int worst = 0;
    for (uint32_t i = 2 * (PERIOD / 4); i < frames * 2; i++) {
        int err = abs(out[i] - (int)lrintf(in[i] * k));
        worst = err > worst ? err : worst;
    }
    CHECK(worst <= 1);
    free(in);
    free(out);
}

static void test_the_gain_comes_back_after_a_loud_burst(void)
{
    const uint32_t burst = 3200;
    const uint32_t quiet = 32000;
    int16_t* loud = stereo_tone(burst, FULL_SCALE, FULL_SCALE);
    int16_t* soft = stereo_tone(quiet, 1000.0f, 1000.0f);
    int16_t* out = malloc((size_t)quiet * 2 * sizeof(int16_t));
    PcmLimiter l = pcm_limiter_run(limiter(), out, loud, burst);
    pcm_limiter_run(l, out, soft, quiet);
    CHECK(out[2 * (PERIOD / 4)] < 1000);
    CHECK(out[2 * (PERIOD / 4)] > 850);
    CHECK(peak(out + 2 * (quiet - PERIOD), PERIOD * 2) == 1500);
    free(loud);
    free(soft);
    free(out);
}

static void test_a_steady_loud_level_settles_at_the_ceiling(void)
{
    const uint32_t burst = 1000;
    const uint32_t steady = 40000;
    int16_t* loud = stereo_tone(burst, FULL_SCALE, FULL_SCALE);
    int16_t* level = malloc((size_t)steady * 2 * sizeof(int16_t));
    int16_t* out = malloc((size_t)steady * 2 * sizeof(int16_t));
    for (uint32_t i = 0; i < steady * 2; i++) {
        level[i] = 20000;
    }
    PcmLimiter l = pcm_limiter_run(limiter(), out, loud, burst);
    pcm_limiter_run(l, out, level, steady);
    CHECK(out[2 * steady - 1] >= CEILING - 1 && out[2 * steady - 1] <= CEILING);
    CHECK(out[0] < CEILING - 2000);
    free(loud);
    free(level);
    free(out);
}

static void test_both_channels_share_the_gain(void)
{
    const uint32_t frames = 4000;
    int16_t* in = stereo_tone(frames, FULL_SCALE, 1000.0f);
    int16_t* out = malloc((size_t)frames * 2 * sizeof(int16_t));
    pcm_limiter_run(limiter(), out, in, frames);
    int right = 0;
    for (uint32_t i = frames - PERIOD; i < frames; i++) {
        int m = abs(out[2 * i + 1]);
        right = m > right ? m : right;
    }
    CHECK(right >= 890 && right <= 892);
    free(in);
    free(out);
}

static void test_split_calls_match_one_call(void)
{
    const uint32_t frames = 6000;
    int16_t* in = stereo_tone(frames, FULL_SCALE, 2000.0f);
    for (uint32_t i = frames / 2; i < frames; i++) {
        in[2 * i] = tone(i, 3000.0f);
    }
    int16_t* whole = malloc((size_t)frames * 2 * sizeof(int16_t));
    int16_t* split = malloc((size_t)frames * 2 * sizeof(int16_t));
    pcm_limiter_run(limiter(), whole, in, frames);
    PcmLimiter l = pcm_limiter_run(limiter(), split, in, 2500);
    pcm_limiter_run(l, split + 2 * 2500, in + 2 * 2500, frames - 2500);
    CHECK(memcmp(whole, split, (size_t)frames * 2 * sizeof(int16_t)) == 0);
    free(in);
    free(whole);
    free(split);
}

static void test_it_runs_in_place(void)
{
    const uint32_t frames = 3000;
    int16_t* in = stereo_tone(frames, FULL_SCALE, -12000.0f);
    int16_t* out = malloc((size_t)frames * 2 * sizeof(int16_t));
    pcm_limiter_run(limiter(), out, in, frames);
    pcm_limiter_run(limiter(), in, in, frames);
    CHECK(memcmp(in, out, (size_t)frames * 2 * sizeof(int16_t)) == 0);
    free(in);
    free(out);
}

static void test_the_most_negative_sample(void)
{
    const int16_t in[2] = { -32768, 32767 };
    int16_t out[2];
    pcm_limiter_run(limiter(), out, in, 1);
    CHECK(out[0] == -CEILING);
    CHECK(out[1] <= CEILING && out[1] >= CEILING - 1);
}

static void test_no_frames_leaves_it_as_it_was(void)
{
    const int16_t in[2] = { 32767, 32767 };
    int16_t out[2];
    PcmLimiter l = pcm_limiter_run(limiter(), out, in, 1);
    PcmLimiter same = pcm_limiter_run(l, NULL, NULL, 0);
    CHECK(same.envelope == l.envelope && same.hold_left == l.hold_left);
}

int main(void)
{
    test_quiet_sound_gets_the_whole_gain();
    test_loud_sound_stays_under_the_ceiling();
    test_a_steady_loud_tone_keeps_its_shape();
    test_the_gain_comes_back_after_a_loud_burst();
    test_a_steady_loud_level_settles_at_the_ceiling();
    test_both_channels_share_the_gain();
    test_split_calls_match_one_call();
    test_it_runs_in_place();
    test_the_most_negative_sample();
    test_no_frames_leaves_it_as_it_was();
    if (failures != 0) {
        fprintf(stderr, "pcm_limiter: %d check(s) failed\n", failures);
        return 1;
    }
    printf("pcm_limiter: all tests passed\n");
    return 0;
}
