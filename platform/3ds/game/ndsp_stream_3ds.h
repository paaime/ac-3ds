#ifndef AC3DS_NDSP_STREAM_3DS_H
#define AC3DS_NDSP_STREAM_3DS_H

#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>

#define NDSP_STREAM_MAX_SLOTS 8

typedef struct {
    uint32_t chunks;
    uint32_t dropped;
    uint32_t underruns;
    uint32_t pending_frames;
} NdspStreamStats;

typedef struct {
    int channel;
    uint32_t slot_count;
    uint32_t slot_frames;
    ndspWaveBuf slots[NDSP_STREAM_MAX_SLOTS];
    int16_t* samples;
    uint32_t next_slot;
    uint32_t peak;
    bool primed;
    uint32_t prime_frames;
    bool paused;
    bool open;
    NdspStreamStats stats;
} NdspStream;

/* After ndspInit. */
bool ndsp_stream_open(NdspStream* s, int channel, float sample_rate, float volume, uint32_t slot_count,
                      uint32_t slot_frames);
/* The channel waits until `frames` are queued before playing, and again whenever it runs dry. */
void ndsp_stream_set_priming(NdspStream* s, uint32_t frames);
void ndsp_stream_set_rate(NdspStream* s, float sample_rate);
void ndsp_stream_close(NdspStream* s);

/* Interleaved stereo, at most slot_frames; one producer thread per stream; false if dropped. */
bool ndsp_stream_write(NdspStream* s, const int16_t* stereo, uint32_t frames);
bool ndsp_stream_wants_chunk(const NdspStream* s, uint32_t target_frames);
NdspStreamStats ndsp_stream_stats(const NdspStream* s);
uint32_t ndsp_stream_take_peak(NdspStream* s);

#endif /* AC3DS_NDSP_STREAM_3DS_H */
