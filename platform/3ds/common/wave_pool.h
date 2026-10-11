#ifndef AC3DS_WAVE_POOL_H
#define AC3DS_WAVE_POOL_H

/* Ring bookkeeping for the ndsp wave buffers (game/ndsp_stream_3ds.c), from a
 * snapshot of their states. */

#include <stdbool.h>
#include <stdint.h>

/* The same values as libctru's NDSP_WBUF_* (checked in ndsp_stream_3ds.c). */
typedef enum {
    WAVE_SLOT_FREE = 0,
    WAVE_SLOT_QUEUED = 1,
    WAVE_SLOT_PLAYING = 2,
    WAVE_SLOT_DONE = 3,
} WaveSlotState;

#define WAVE_POOL_NONE (-1)

/* The first FREE or DONE slot from `start` in ring order, or WAVE_POOL_NONE. */
int wave_pool_next_free(const uint8_t* states, uint32_t count, uint32_t start);

uint32_t wave_pool_pending_frames(const uint8_t* states, const uint32_t* frames, uint32_t count);

/* True while less than `target_frames` is pending and a slot is free. */
bool wave_pool_wants_chunk(const uint8_t* states, const uint32_t* frames, uint32_t count, uint32_t target_frames);

#endif /* AC3DS_WAVE_POOL_H */
