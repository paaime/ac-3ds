/* profiler_3ds.c - frame CPU profile (see profiler_3ds.h), also fed by the PC
 * port's profiler entry points that graph.c calls. svcGetSystemTick is a system
 * call, so only a dozen points per frame are timed, never per draw. */
#include <3ds.h>
#include <stdio.h>

#include "pc_profiler.h"
#include "prof_window.h"
#include "profiler_3ds.h"

#define WINDOW_FRAMES   60
#define LOG_WINDOWS     10  /* 10-20 s between log lines */

int g_pc_profile_enabled = AC3DS_DEBUG; /* graph.c times its loop when set */
int g_pc_profile_interval = WINDOW_FRAMES;

static ProfWindow s_window;
static Prof3dsReport s_report;
static uint64_t s_frame_start; /* also the end of the last frame */
static uint64_t s_window_start;
static uint32_t s_mixer_ticks; /* written by core 2 */
static uint32_t s_windows;

static const char* const NAMES[PROF_COUNT] = {
    "logic", "dl", "emu64", "wait", "end", "flush", "tex", "audio", "status",
};

uint64_t prof3ds_now(void)
{
    return svcGetSystemTick();
}

void prof3ds_add_ticks(Prof3dsSlot slot, uint64_t ticks)
{
    s_window = prof_window_add(s_window, (unsigned)slot, ticks);
}

void prof3ds_add(Prof3dsSlot slot, uint64_t start)
{
    prof3ds_add_ticks(slot, svcGetSystemTick() - start);
}

void prof3ds_mixer_add(uint32_t ticks)
{
    __atomic_fetch_add(&s_mixer_ticks, ticks, __ATOMIC_RELAXED);
}

static void log_report(const Prof3dsReport* r)
{
    char line[256];
    int n = snprintf(line, sizeof(line), "[prof] %.1f ms/frame:", r->frame_ms);
    for (int i = 0; i < PROF_COUNT && n > 0 && (size_t)n < sizeof(line); i++) {
        n += snprintf(line + n, sizeof(line) - (size_t)n, " %s %.2f", NAMES[i], r->ms[i]);
    }
    fprintf(stderr, "%s | mixer %.0f%% of core 2\n", line, r->mixer_percent);
}

static void close_window(uint64_t now)
{
    Prof3dsReport r;
    for (int i = 0; i < PROF_COUNT; i++) {
        r.ms[i] = (float)prof_window_ms(s_window, (unsigned)i, SYSCLOCK_ARM11);
    }
    r.frame_ms = (float)prof_window_frame_ms(s_window, SYSCLOCK_ARM11);
    uint32_t mixer = __atomic_exchange_n(&s_mixer_ticks, 0, __ATOMIC_RELAXED);
    r.mixer_percent = (float)prof_share_percent(mixer, now - s_window_start);
    r.frames = prof_window_frames(s_window);
    s_report = r;
    s_window = prof_window_create();
    s_window_start = now;
    if (AC3DS_DEBUG && ++s_windows % LOG_WINDOWS == 0) {
        log_report(&r);
    }
}

void prof3ds_frame(void)
{
    uint64_t now = svcGetSystemTick();
    if (s_frame_start == 0) { /* the first frame only starts the clock */
        s_frame_start = now;
        s_window_start = now;
        return;
    }
    s_window = prof_window_end_frame(s_window, now - s_frame_start);
    s_frame_start = now;
    if (prof_window_frames(s_window) >= WINDOW_FRAMES) {
        close_window(now);
    }
}

Prof3dsReport prof3ds_report(void)
{
    return s_report;
}

/* ---- the PC port's profiler API (graph.c) ------------------------------------- */

void pc_profiler_begin_frame(void) {}

void pc_profiler_end_frame(double frame_ms, int audio_fill)
{
    (void)frame_ms;
    (void)audio_fill;
}

void pc_profiler_add_time_slow(PCProfilerTimer timer, Uint64 start)
{
    switch (timer) {
    case PC_PROF_TIMER_GAME_LOGIC:
        /* NES games run whole frames inside game_main (famicom_emu_main calls
         * VIWaitForRetrace): count logic only since the last frame closed. */
        prof3ds_add(PROF_LOGIC, start > s_frame_start ? start : s_frame_start);
        break;
    case PC_PROF_TIMER_DRAW_FINISH: prof3ds_add(PROF_DRAW_FINISH, start); break;
    case PC_PROF_TIMER_EMU64: prof3ds_add(PROF_EMU64, start); break;
    case PC_PROF_TIMER_AUDIO_FRAME: prof3ds_add(PROF_AUDIO_GAME, start); break;
    default: break; /* JW_FRAME spans the wait and emu64; the rest are GL-only */
    }
}

void pc_profiler_add_count_draw_slow(int vertices, int indices)
{
    (void)vertices;
    (void)indices;
}

void pc_profiler_add_count_flush_slow(void) {}
void pc_profiler_add_count_shader_switch_slow(void) {}
void pc_profiler_add_count_uniform_slow(void) {}
void pc_profiler_add_count_uniform_lookup_slow(void) {}
void pc_profiler_add_count_texture_bind_slow(void) {}

void pc_profiler_add_count_buffer_upload_slow(size_t bytes)
{
    (void)bytes;
}

void pc_profiler_add_count_state_change_slow(void) {}

void pc_profiler_add_dirty_mask_slow(unsigned int dirty)
{
    (void)dirty;
}
