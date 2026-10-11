#ifndef AC3DS_NDSP_STREAM_3DS_H
#define AC3DS_NDSP_STREAM_3DS_H

/* One ndsp channel fed with stereo PCM16 chunks through a ring of wave buffers
 * in linear memory (ndsp_stream_3ds.c). One producer thread per stream; ndsp
 * must be initialised first. */

#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>

#define NDSP_STREAM_MAX_SLOTS 8

typedef struct {
    uint32_t chunks;         /* chunks queued since the stream opened */
    uint32_t dropped;        /* chunks dropped because every buffer was busy */
    uint32_t underruns;      /* times the queue had run dry when a chunk arrived */
    uint32_t pending_frames; /* frames waiting to play now */
} NdspStreamStats;

typedef struct {
    int channel;
    uint32_t slot_count;
    uint32_t slot_frames; /* capacity of each buffer, in stereo frames */
    ndspWaveBuf slots[NDSP_STREAM_MAX_SLOTS];
    int16_t* samples; /* linear memory, slot_count * slot_frames stereo frames */
    uint32_t next_slot;
    uint32_t peak;  /* loudest sample since the last take, written by the producer */
    bool primed;    /* a chunk has queued behind another: dry queues now count */
    uint32_t prime_frames; /* 0, or: stay paused until this much is queued */
    bool paused;
    bool open;
    NdspStreamStats stats;
} NdspStream;

bool ndsp_stream_open(NdspStream* s, int channel, float sample_rate, float volume, uint32_t slot_count,
                      uint32_t slot_frames);
/* For a producer whose chunks arrive irregularly: the channel waits until
 * `frames` are queued before it plays, and again whenever it runs dry. */
void ndsp_stream_set_priming(NdspStream* s, uint32_t frames);
void ndsp_stream_set_rate(NdspStream* s, float sample_rate);
/* Stops the channel, lets the DSP finish with the buffers, frees them. */
void ndsp_stream_close(NdspStream* s);

/* Queues `frames` interleaved stereo frames (at most slot_frames); false if dropped. */
bool ndsp_stream_write(NdspStream* s, const int16_t* stereo, uint32_t frames);
/* True while less than `target_frames` is waiting to play and a buffer is free. */
bool ndsp_stream_wants_chunk(const NdspStream* s, uint32_t target_frames);
NdspStreamStats ndsp_stream_stats(const NdspStream* s);
/* The loudest sample queued since the last call (0..32768), then resets. */
uint32_t ndsp_stream_take_peak(NdspStream* s);

#endif /* AC3DS_NDSP_STREAM_3DS_H */
