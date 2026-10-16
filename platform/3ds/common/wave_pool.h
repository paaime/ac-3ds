#ifndef AC3DS_WAVE_POOL_H
#define AC3DS_WAVE_POOL_H

#include <stdbool.h>
#include <stdint.h>

/* libctru's NDSP_WBUF_* values (checked in ndsp_stream_3ds.c). */
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

bool wave_pool_wants_chunk(const uint8_t* states, const uint32_t* frames, uint32_t count, uint32_t target_frames);

#endif /* AC3DS_WAVE_POOL_H */
