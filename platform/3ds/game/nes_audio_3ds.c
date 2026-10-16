#include "pc_platform.h"

#include <stdio.h>

#include <3ds.h>

#include "audio_out_3ds.h"
#include "feed_rate.h"
#include "nes_audio_3ds.h"
#include "ndsp_stream_3ds.h"
#include "pcm_stereo.h"

#define FULL_VOLUME  1.0f
#define NES_CHANNEL  1 /* the game's mixer has channel 0 (audio_out_3ds.c) */
#define SLOT_COUNT   6 /* ~100 ms: one ~545-sample chunk per NES frame at 60 Hz */
#define SLOT_FRAMES  1024
#define MAX_SAMPLES  SLOT_FRAMES
#define PRIME_FRAMES 3
#define PRIME_RATE   61 /* a little over the NES's 60.1 Hz, so three chunks are enough */

static NdspStream s_stream;
static uint32_t s_rate;
static FeedRate s_feed;
static float s_playing_rate;
static int16_t s_stereo[2 * MAX_SAMPLES];

void nes_audio_write(const int16_t* mono, uint32_t samples, uint32_t sample_rate)
{
    if (!audio_out_is_open() || mono == NULL || samples == 0 || sample_rate == 0) {
        return;
    }
    if (s_stream.open && sample_rate != s_rate) {
        ndsp_stream_close(&s_stream);
    }
    if (!s_stream.open) {
        s_rate = sample_rate;
        if (!ndsp_stream_open(&s_stream, NES_CHANNEL, (float)sample_rate, FULL_VOLUME, SLOT_COUNT, SLOT_FRAMES)) {
            return;
        }
        ndsp_stream_set_priming(&s_stream, sample_rate * PRIME_FRAMES / PRIME_RATE);
        s_feed = feed_rate_create((float)sample_rate, SYSCLOCK_ARM11);
        s_playing_rate = feed_rate_value(s_feed);
        fprintf(stderr, "[nes] sound: %lu Hz on channel %d\n", (unsigned long)sample_rate, NES_CHANNEL);
    }
    uint32_t n = samples < MAX_SAMPLES ? samples : MAX_SAMPLES;
    pcm_mono_to_stereo(s_stereo, mono, n);
    ndsp_stream_write(&s_stream, s_stereo, n);
    s_feed = feed_rate_add(s_feed, n, svcGetSystemTick());
    if (feed_rate_value(s_feed) != s_playing_rate) {
        s_playing_rate = feed_rate_value(s_feed);
        ndsp_stream_set_rate(&s_stream, s_playing_rate);
    }
}

void nes_audio_stop(void)
{
    if (s_stream.open) {
        NdspStreamStats st = ndsp_stream_stats(&s_stream);
        fprintf(stderr, "[nes] sound: %lu chunks, %lu underruns, %lu dropped\n", (unsigned long)st.chunks,
                (unsigned long)st.underruns, (unsigned long)st.dropped);
        ndsp_stream_close(&s_stream);
    }
}
