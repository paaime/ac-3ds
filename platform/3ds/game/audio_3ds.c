/* audio_3ds.c - the audio interface (replaces pc/src/pc_audio.c). jaudio's
 * chunks go to ndsp, mixed by a core-2 thread or the game thread. */
#include "pc_platform.h"

#include <3ds/svc.h> /* not <3ds.h>: see os_3ds.c */
#include <stdio.h>

#include "jaudio_NES/rate.h"

#include "apt_trace_3ds.h"
#include "audio_3ds.h"
#include "audio_out_3ds.h"
#include "audio_pacer.h"
#include "audio_thread_3ds.h"
#include "nes_audio_3ds.h"
#include "gc_clock.h"
#include "pcm_level.h"
#include "profiler_3ds.h"
#include "settings_3ds.h"

#define FULL_VOLUME 1.5f /* the console's own slider sets the volume */
#define BYTES_PER_FRAME     4   /* stereo s16 */
#define CHUNK_FRAMES        560 /* jaudio's DAC_SIZE / 2: one mixer call */
#define THREAD_TARGET       (3 * CHUNK_FRAMES) /* ~52 ms: the thread wakes every ~5 ms */
#define GAME_THREAD_TARGET  (6 * CHUNK_FRAMES) /* ~105 ms: covers a slow frame */
#define MAX_MIXES_PER_FILL  8                  /* one per wave buffer */
#define MS_PER_SECOND       1000u
#define SOUND_MODE_STEREO   1 /* OS_SOUND_MODE_STEREO; OSRtc.h's statics would warn here */

typedef enum {
    AUDIO_MODE_OFF,         /* before the game starts its mixer */
    AUDIO_MODE_SILENT,      /* no ndsp: mix at clock pace, drop the output */
    AUDIO_MODE_GAME_THREAD,
    AUDIO_MODE_THREAD,
} AudioMode;

/* audiothread.c (TARGET_PC): one mixer step, ending in AIInitDMA. */
void pc_audio_process_frame(void);

static AudioMode s_mode = AUDIO_MODE_OFF;
static AudioOutStatus s_out_status = AUDIO_OUT_NO_FIRMWARE;
static bool s_out_tried;
static uint32_t s_mixed_frames; /* set by AIInitDMA */
static AudioPacer s_pacer;
static u64 s_start_tick;
static u32 s_sound_mode = SOUND_MODE_STEREO;

/* ---- OS sound mode (pc_stubs.c's copies are renamed away, CMakeLists.txt) --- */

/* The PC stub answers mono, which forces mono output: report
 * the IPL's default and let the game's own sound option choose. */
u32 OSGetSoundMode(void);
void OSSetSoundMode(u32 mode);

u32 OSGetSoundMode(void)
{
    return s_sound_mode;
}

void OSSetSoundMode(u32 mode)
{
    s_sound_mode = mode;
}

/* ---- AI --------------------------------------------------------------------- */

static void start_mixer(void);

void AIInit(u8* stack)
{
    (void)stack;
    if (s_out_tried) {
        return;
    }
    s_out_tried = true;
    s_out_status = audio_out_init(JAC_DAC_RATE, FULL_VOLUME);
    apt_trace3ds_after_libs(); /* after ndspInit: its markers run before ndsp's hooks */
    if (s_out_status != AUDIO_OUT_OK) {
        fprintf(stderr, "[audio] no sound: %s\n", audio_out_status_text(s_out_status));
        return;
    }
    fprintf(stderr, "[audio] DSP firmware from the %s\n", audio_out_firmware_source());
    if (s_mode == AUDIO_MODE_SILENT) {
        start_mixer(); /* the mixer was already running without output */
    }
}

void AIInitDMA(u32 addr, u32 size)
{
    uint32_t frames = size / BYTES_PER_FRAME;
    audio_out_write((const int16_t*)(uintptr_t)addr, frames);
    s_mixed_frames += frames;
}

void AIStartDMA(void) {}

void AISetDSPSampleRate(u32 rate)
{
    (void)rate; /* jaudio sets 32 kHz (JAC_AI_SETTING); the output runs at JAC_DAC_RATE */
}

/* DVD audio streaming: unused, as in pc_audio.c. */
u32 AIGetStreamTrigger(void)
{
    return 0;
}

u32 AIGetStreamSampleCount(void)
{
    return 0;
}

void AIResetStreamSampleCount(void) {}

void AISetStreamPlayState(u32 state)
{
    (void)state;
}

void AISetStreamSampleRate(u32 rate)
{
    (void)rate;
}

void AISetStreamVolLeft(u8 vol)
{
    (void)vol;
}

void AISetStreamVolRight(u8 vol)
{
    (void)vol;
}

/* ---- mixer drivers ------------------------------------------------------------- */

/* Runs the mixer until `target` frames are queued or every buffer is busy. */
static void fill_to(uint32_t target)
{
    for (int i = 0; i < MAX_MIXES_PER_FILL && audio_out_wants_chunk(target); i++) {
        s_mixed_frames = 0;
        pc_audio_process_frame();
        if (s_mixed_frames == 0) {
            break; /* the first step only mixes ahead */
        }
    }
}

static void fill_from_thread(void)
{
    u64 start = svcGetSystemTick();
    fill_to(THREAD_TARGET);
    prof3ds_mixer_add((uint32_t)(svcGetSystemTick() - start));
}

/* No output: keep jaudio moving at real-time pace. */
static void pace_silently(void)
{
    u64 elapsed = svcGetSystemTick() - s_start_tick;
    s_pacer = audio_pacer_advance_to(s_pacer, gc_clock_convert(elapsed, N3DS_SYSTEM_TICK_HZ, (uint32_t)JAC_DAC_RATE));
    for (int i = 0; i < AUDIO_PACER_MAX_MIXES && audio_pacer_behind(s_pacer); i++) {
        s_mixed_frames = 0;
        pc_audio_process_frame();
        if (s_mixed_frames == 0) {
            break;
        }
        s_pacer = audio_pacer_produced(s_pacer, s_mixed_frames);
    }
}

static bool single_thread_requested(void)
{
    return settings3ds_on(SETTING_ONE_CORE_AUDIO);
}

static void start_mixer(void)
{
    if (!single_thread_requested() && audio_thread_start(fill_from_thread)) {
        s_mode = AUDIO_MODE_THREAD;
        fprintf(stderr, "[audio] mixer thread on core %d\n", audio_thread_core());
        return;
    }
    s_mode = AUDIO_MODE_GAME_THREAD;
    fprintf(stderr, "[audio] mixing on the game thread (%s)\n",
            single_thread_requested() ? "the one-core setting is on" : "core 2 unavailable");
}

/* Called at the end of StartAudioThread: the mixer is ready to run. */
void pc_audio_start_producer_thread(void)
{
    if (s_mode != AUDIO_MODE_OFF) {
        return;
    }
    if (audio_out_is_open()) {
        start_mixer();
        return;
    }
    if (!s_out_tried) {
        fprintf(stderr, "[audio] no sound: the game never called AIInit\n");
    }
    s_pacer = audio_pacer_create((uint32_t)JAC_DAC_RATE);
    s_start_tick = svcGetSystemTick();
    s_mode = AUDIO_MODE_SILENT;
}

void audio3ds_frame(void)
{
    switch (s_mode) {
    case AUDIO_MODE_SILENT:
        pace_silently();
        break;
    case AUDIO_MODE_GAME_THREAD:
        fill_to(GAME_THREAD_TARGET);
        break;
    case AUDIO_MODE_OFF:
    case AUDIO_MODE_THREAD:
        break;
    }
}

Audio3dsStatus audio3ds_status(void)
{
    AudioOutStats out = audio_out_stats();
    Audio3dsStatus st = { s_mode == AUDIO_MODE_THREAD || s_mode == AUDIO_MODE_GAME_THREAD,
                          audio_thread_core(),
                          (uint32_t)((uint64_t)out.pending_frames * MS_PER_SECOND / (uint32_t)JAC_DAC_RATE),
                          out.underruns,
                          out.dropped,
                          pcm_peak_percent(audio_out_take_peak()) };
    return st;
}

void pc_audio_shutdown(void)
{
    audio_thread_stop();
    nes_audio_stop(); /* an NES game's channel, before ndspExit */
    if (audio_out_is_open()) {
        AudioOutStats out = audio_out_stats();
        fprintf(stderr, "[audio] %lu chunks played, %lu underruns, %lu dropped\n", (unsigned long)out.chunks,
                (unsigned long)out.underruns, (unsigned long)out.dropped);
    }
    audio_out_exit();
    s_mode = AUDIO_MODE_OFF;
}

void pc_audio_set_paused(int paused)
{
    (void)paused;
}

int pc_audio_get_buffer_fill(void)
{
    return (int)audio_out_stats().pending_frames * 2; /* samples, as pc_audio.c counts */
}

int pc_audio_is_active(void)
{
    return audio_out_is_open();
}
