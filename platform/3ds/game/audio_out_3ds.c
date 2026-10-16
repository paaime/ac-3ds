#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio_out_3ds.h"
#include "dsp_firmware_3ds.h"
#include "ndsp_stream_3ds.h"

#define GAME_CHANNEL       0
#define SLOT_COUNT         8
#define SLOT_FRAMES        1024 /* the mixer writes 560 per chunk */
#define COMPONENT_ALL_MASK 0xFF

static NdspStream s_game;
static DspFirmware s_firmware; /* kept until ndspExit: ndsp reloads it after sleep/HOME */
static LightEvent s_dsp_frame;
static bool s_open;

static void forget_firmware(void)
{
    ndspUseComponent(NULL, 0, 0, 0); /* libctru keeps the pointer otherwise */
    free(s_firmware.data);
    s_firmware.data = NULL;
}

static void on_dsp_frame(void* data)
{
    (void)data;
    LightEvent_Signal(&s_dsp_frame);
}

AudioOutStatus audio_out_init(float sample_rate, float volume)
{
    if (s_open) {
        return AUDIO_OUT_OK;
    }
    s_firmware = dsp_firmware_load();
    if (s_firmware.data == NULL) {
        return AUDIO_OUT_NO_FIRMWARE;
    }
    ndspUseComponent(s_firmware.data, s_firmware.size, COMPONENT_ALL_MASK, COMPONENT_ALL_MASK);
    if (R_FAILED(ndspInit())) {
        forget_firmware();
        return AUDIO_OUT_DSP_FAILED;
    }
    if (!ndsp_stream_open(&s_game, GAME_CHANNEL, sample_rate, volume, SLOT_COUNT, SLOT_FRAMES)) {
        ndspExit();
        forget_firmware();
        return AUDIO_OUT_NO_MEMORY;
    }
    LightEvent_Init(&s_dsp_frame, RESET_ONESHOT);
    ndspSetCallback(on_dsp_frame, NULL);
    s_open = true;
    return AUDIO_OUT_OK;
}

void audio_out_exit(void)
{
    if (!s_open) {
        return;
    }
    s_open = false;
    ndspSetCallback(NULL, NULL);
    ndsp_stream_close(&s_game);
    ndspExit();
    forget_firmware();
}

bool audio_out_is_open(void)
{
    return s_open;
}

bool audio_out_write(const int16_t* stereo, uint32_t frames)
{
    return ndsp_stream_write(&s_game, stereo, frames);
}

bool audio_out_wants_chunk(uint32_t target_frames)
{
    return ndsp_stream_wants_chunk(&s_game, target_frames);
}

void audio_out_wait(int64_t timeout_ns)
{
    LightEvent_WaitTimeout(&s_dsp_frame, timeout_ns);
}

void audio_out_wake(void)
{
    LightEvent_Signal(&s_dsp_frame);
}

AudioOutStats audio_out_stats(void)
{
    NdspStreamStats s = ndsp_stream_stats(&s_game);
    AudioOutStats stats = { s.chunks, s.dropped, s.underruns, s.pending_frames };
    return stats;
}

uint32_t audio_out_take_peak(void)
{
    return ndsp_stream_take_peak(&s_game);
}

const char* audio_out_firmware_source(void)
{
    if (!s_open) {
        return NULL;
    }
    return s_firmware.source == DSP_FIRMWARE_FROM_SD ? "SD card" : "launcher";
}

const char* audio_out_status_text(AudioOutStatus status)
{
    switch (status) {
    case AUDIO_OUT_OK:
        return "ok";
    case AUDIO_OUT_NO_FIRMWARE:
        return "no valid DSP firmware: dump it to sdmc:/3ds/dspfirm.cdc (Rosalina menu)";
    case AUDIO_OUT_DSP_FAILED:
        return "the DSP did not start (ndspInit failed)";
    case AUDIO_OUT_NO_MEMORY:
        return "out of linear memory for the sound buffers";
    }
    return "unknown";
}
