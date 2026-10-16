#include "wave_pool.h"

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

#define CHUNK 560 /* stereo frames one mixer call produces */

enum { F = WAVE_SLOT_FREE, Q = WAVE_SLOT_QUEUED, P = WAVE_SLOT_PLAYING, D = WAVE_SLOT_DONE };

static void test_next_free_starts_at_start(void)
{
    const uint8_t s[4] = { F, F, F, F };
    CHECK(wave_pool_next_free(s, 4, 0) == 0);
    CHECK(wave_pool_next_free(s, 4, 2) == 2);
}

static void test_next_free_skips_busy_slots_and_wraps(void)
{
    const uint8_t s[4] = { F, Q, P, Q };
    CHECK(wave_pool_next_free(s, 4, 1) == 0);
    const uint8_t t[4] = { Q, P, F, Q };
    CHECK(wave_pool_next_free(t, 4, 0) == 2);
}

static void test_done_slots_are_reusable(void)
{
    const uint8_t s[3] = { P, D, Q };
    CHECK(wave_pool_next_free(s, 3, 0) == 1);
}

static void test_next_free_none_when_all_busy(void)
{
    const uint8_t s[3] = { Q, P, Q };
    CHECK(wave_pool_next_free(s, 3, 0) == WAVE_POOL_NONE);
    CHECK(wave_pool_next_free(s, 0, 0) == WAVE_POOL_NONE);
}

static void test_next_free_start_past_the_end_wraps(void)
{
    const uint8_t s[4] = { F, Q, Q, Q };
    CHECK(wave_pool_next_free(s, 4, 5) == 0);
}

static void test_pending_counts_queued_and_playing_only(void)
{
    const uint8_t s[5] = { F, Q, P, D, Q };
    const uint32_t frames[5] = { 100, CHUNK, CHUNK, 7, 300 };
    CHECK(wave_pool_pending_frames(s, frames, 5) == 2 * CHUNK + 300);
    CHECK(wave_pool_pending_frames(s, frames, 0) == 0);
}

static void test_wants_chunk_below_target_with_a_free_slot(void)
{
    const uint8_t s[4] = { Q, P, F, D };
    const uint32_t frames[4] = { CHUNK, CHUNK, CHUNK, CHUNK };
    CHECK(wave_pool_wants_chunk(s, frames, 4, 3 * CHUNK));
}

static void test_no_chunk_at_target(void)
{
    const uint8_t s[4] = { Q, P, Q, F };
    const uint32_t frames[4] = { CHUNK, CHUNK, CHUNK, 0 };
    CHECK(!wave_pool_wants_chunk(s, frames, 4, 3 * CHUNK));
}

static void test_no_chunk_without_a_free_slot(void)
{
    const uint8_t s[2] = { Q, P };
    const uint32_t frames[2] = { 10, 10 };
    CHECK(!wave_pool_wants_chunk(s, frames, 2, 3 * CHUNK));
}

int main(void)
{
    test_next_free_starts_at_start();
    test_next_free_skips_busy_slots_and_wraps();
    test_done_slots_are_reusable();
    test_next_free_none_when_all_busy();
    test_next_free_start_past_the_end_wraps();
    test_pending_counts_queued_and_playing_only();
    test_wants_chunk_below_target_with_a_free_slot();
    test_no_chunk_at_target();
    test_no_chunk_without_a_free_slot();
    if (failures == 0) {
        printf("wave_pool: all tests passed\n");
    }
    return failures == 0 ? 0 : 1;
}
