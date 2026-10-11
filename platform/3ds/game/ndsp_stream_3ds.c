/* ndsp_stream_3ds.c - a stereo PCM16 stream on one ndsp channel (see
 * ndsp_stream_3ds.h). */
#include <string.h>

#include "ndsp_stream_3ds.h"
#include "pcm_level.h"
#include "wave_pool.h"

#define BYTES_PER_FRAME 4 /* stereo s16 */
#define DRAIN_POLL_NS   (1000LL * 1000) /* 1 ms */
#define DRAIN_POLLS     20              /* a DSP frame is ~5 ms */

_Static_assert((int)WAVE_SLOT_FREE == (int)NDSP_WBUF_FREE, "wave_pool states follow libctru");
_Static_assert((int)WAVE_SLOT_QUEUED == (int)NDSP_WBUF_QUEUED, "wave_pool states follow libctru");
_Static_assert((int)WAVE_SLOT_PLAYING == (int)NDSP_WBUF_PLAYING, "wave_pool states follow libctru");
_Static_assert((int)WAVE_SLOT_DONE == (int)NDSP_WBUF_DONE, "wave_pool states follow libctru");

static void snapshot(const NdspStream* s, uint8_t* states, uint32_t* frames)
{
    for (uint32_t i = 0; i < s->slot_count; i++) {
        states[i] = *(volatile const u8*)&s->slots[i].status; /* written by ndsp's thread */
        frames[i] = s->slots[i].nsamples;
    }
}

static void setup_channel(int channel, float sample_rate, float volume)
{
    float mix[12] = { 0 };
    mix[0] = volume; /* front left */
    mix[1] = volume; /* front right */
    /* The output mode (mono/stereo/surround) stays the system setting ndspInit read. */
    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(channel, sample_rate);
    ndspChnSetFormat(channel, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetMix(channel, mix);
}

bool ndsp_stream_open(NdspStream* s, int channel, float sample_rate, float volume, uint32_t slot_count,
                      uint32_t slot_frames)
{
    if (s->open || slot_count == 0 || slot_count > NDSP_STREAM_MAX_SLOTS || slot_frames == 0) {
        return s->open;
    }
    memset(s, 0, sizeof(*s));
    s->samples = linearAlloc((size_t)slot_count * slot_frames * BYTES_PER_FRAME);
    if (s->samples == NULL) {
        return false;
    }
    s->channel = channel;
    s->slot_count = slot_count;
    s->slot_frames = slot_frames;
    for (uint32_t i = 0; i < slot_count; i++) {
        s->slots[i].data_pcm16 = s->samples + (size_t)i * slot_frames * 2;
        s->slots[i].status = NDSP_WBUF_FREE;
    }
    setup_channel(channel, sample_rate, volume);
    s->open = true;
    return true;
}

void ndsp_stream_set_priming(NdspStream* s, uint32_t frames)
{
    s->prime_frames = frames;
    s->paused = frames != 0;
    ndspChnSetPaused(s->channel, s->paused);
}

void ndsp_stream_set_rate(NdspStream* s, float sample_rate)
{
    if (s->open) {
        ndspChnSetRate(s->channel, sample_rate);
    }
}

void ndsp_stream_close(NdspStream* s)
{
    if (!s->open) {
        return;
    }
    s->open = false;
    ndspChnWaveBufClear(s->channel);
    /* The DSP may still read the current buffer for one more frame. */
    for (int i = 0; i < DRAIN_POLLS && ndspChnIsPlaying(s->channel); i++) {
        svcSleepThread(DRAIN_POLL_NS);
    }
    ndspChnReset(s->channel);
    linearFree(s->samples);
    s->samples = NULL;
}

static void update_priming(NdspStream* s, bool was_dry)
{
    if (s->prime_frames == 0) {
        return;
    }
    if (!s->paused && was_dry) {
        s->paused = true;
        ndspChnSetPaused(s->channel, true);
    } else if (s->paused) {
        uint8_t states[NDSP_STREAM_MAX_SLOTS];
        uint32_t pending[NDSP_STREAM_MAX_SLOTS];
        snapshot(s, states, pending);
        if (wave_pool_pending_frames(states, pending, s->slot_count) >= s->prime_frames) {
            s->paused = false;
            ndspChnSetPaused(s->channel, false);
        }
    }
}


static void raise_peak(NdspStream* s, const int16_t* samples, uint32_t count)
{
    uint32_t peak = pcm_peak(samples, count);
    if (peak > __atomic_load_n(&s->peak, __ATOMIC_RELAXED)) {
        __atomic_store_n(&s->peak, peak, __ATOMIC_RELAXED);
    }
}

bool ndsp_stream_write(NdspStream* s, const int16_t* stereo, uint32_t frames)
{
    if (!s->open || stereo == NULL || frames == 0) {
        return false;
    }
    uint8_t states[NDSP_STREAM_MAX_SLOTS];
    uint32_t pending[NDSP_STREAM_MAX_SLOTS];
    snapshot(s, states, pending);
    int slot = wave_pool_next_free(states, s->slot_count, s->next_slot);
    if (slot == WAVE_POOL_NONE) {
        s->stats.dropped++;
        return false;
    }
    bool dry = wave_pool_pending_frames(states, pending, s->slot_count) == 0;
    if (dry && s->primed && !s->paused) {
        s->stats.underruns++;
    }
    s->primed = s->primed || !dry; /* the first chunk drains while the producer starts */
    uint32_t n = frames < s->slot_frames ? frames : s->slot_frames;
    ndspWaveBuf* wb = &s->slots[slot];
    memcpy(wb->data_pcm16, stereo, (size_t)n * BYTES_PER_FRAME);
    DSP_FlushDataCache(wb->data_pcm16, n * BYTES_PER_FRAME);
    raise_peak(s, wb->data_pcm16, n * 2);
    wb->nsamples = n;
    wb->looping = false;
    ndspChnWaveBufAdd(s->channel, wb);
    s->next_slot = (uint32_t)(slot + 1) % s->slot_count;
    s->stats.chunks++;
    update_priming(s, dry && s->primed);
    return true;
}

bool ndsp_stream_wants_chunk(const NdspStream* s, uint32_t target_frames)
{
    if (!s->open) {
        return false;
    }
    uint8_t states[NDSP_STREAM_MAX_SLOTS];
    uint32_t frames[NDSP_STREAM_MAX_SLOTS];
    snapshot(s, states, frames);
    return wave_pool_wants_chunk(states, frames, s->slot_count, target_frames);
}

NdspStreamStats ndsp_stream_stats(const NdspStream* s)
{
    NdspStreamStats stats = s->stats;
    if (s->open) {
        uint8_t states[NDSP_STREAM_MAX_SLOTS];
        uint32_t frames[NDSP_STREAM_MAX_SLOTS];
        snapshot(s, states, frames);
        stats.pending_frames = wave_pool_pending_frames(states, frames, s->slot_count);
    }
    return stats;
}

uint32_t ndsp_stream_take_peak(NdspStream* s)
{
    return __atomic_exchange_n(&s->peak, 0, __ATOMIC_RELAXED);
}
