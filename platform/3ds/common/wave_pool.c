#include "wave_pool.h"

static bool is_reusable(uint8_t state)
{
    return state == WAVE_SLOT_FREE || state == WAVE_SLOT_DONE;
}

static bool is_pending(uint8_t state)
{
    return state == WAVE_SLOT_QUEUED || state == WAVE_SLOT_PLAYING;
}

int wave_pool_next_free(const uint8_t* states, uint32_t count, uint32_t start)
{
    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot = (start + i) % count;
        if (is_reusable(states[slot])) {
            return (int)slot;
        }
    }
    return WAVE_POOL_NONE;
}

uint32_t wave_pool_pending_frames(const uint8_t* states, const uint32_t* frames, uint32_t count)
{
    uint32_t total = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (is_pending(states[i])) {
            total += frames[i];
        }
    }
    return total;
}

bool wave_pool_wants_chunk(const uint8_t* states, const uint32_t* frames, uint32_t count, uint32_t target_frames)
{
    return wave_pool_pending_frames(states, frames, count) < target_frames &&
           wave_pool_next_free(states, count, 0) != WAVE_POOL_NONE;
}
