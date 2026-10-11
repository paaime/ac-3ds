/* vi_3ds.c - video interface and the game's frame hooks on 3DS (replaces
 * pc/src/pc_vi.c, and pc_gx_begin_frame from pc/src/pc_gx.c). C3D_FrameBegin
 * paces frames at 60 Hz, the GC's NTSC rate, so there is no frame limiter. */
#include "pc_platform.h"

#include "acres_3ds.h"
#include "audio_3ds.h"
#include "frame_3ds.h"
#include "os_3ds.h"
#include "profiler_3ds.h"
#include "settings_3ds.h"
#include "shop_3ds.h"
#include "trace_3ds.h"
#include "test_flags_3ds.h"
#include "weeds_3ds.h"
#include "vi_3ds.h"

#define FRAME_RATE_HZ 60

u32 g_frame_limiter = FRAME_RATE_HZ;
u32 pc_frame_counter = 0;
int pc_gx_draw_call_count = 0;

static u32 s_retrace_count;
static Emu64FrameCounts s_last_emu64;
static void* s_pre_retrace_callback;
static void* s_post_retrace_callback;

/* ---- frame hooks ------------------------------------------------------------ */

/* Called by JW_BeginFrame (jsyswrap.cpp). */
void pc_gx_begin_frame(void)
{
    pc_emu64_frame_cmds = 0;
    pc_emu64_frame_noop_cmds = 0;
    pc_emu64_frame_tri_cmds = 0;
    pc_emu64_frame_vtx_cmds = 0;
    pc_emu64_frame_dl_cmds = 0;
    pc_emu64_frame_cull_visible = 0;
    pc_emu64_frame_cull_rejected = 0;
    pc_gx_draw_call_count = 0;
    g_pc_widescreen_stretch = 0;
    static uint32_t frames_begun;
    trace3ds_frame(++frames_begun); /* drawn frames, from 1 */
    frame3ds_begin();
}

Emu64FrameCounts vi3ds_last_emu64_counts(void)
{
    return s_last_emu64;
}

void VIWaitForRetrace(void)
{
    if (frame3ds_is_open()) {
        Emu64FrameCounts c = { pc_emu64_frame_cmds, pc_emu64_frame_tri_cmds, pc_emu64_frame_vtx_cmds,
                               pc_emu64_frame_dl_cmds, pc_emu64_frame_cull_rejected };
        s_last_emu64 = c;
    }
    frame3ds_end();
    /* After the submit: a HOME suspend in aptMainLoop must not land mid-frame. */
    trace3ds_mark("between frames: HOME and input (aptMainLoop, hidScanInput)");
    if (!pc_platform_poll_events()) {
        g_pc_running = 0;
    }
    os3ds_clock_frame(); /* after a sleep inside aptMainLoop: the town's clock catches up */
    trace3ds_mark("between frames: sound, test hooks, status");
    audio3ds_frame();
    test_flags_frame();
    shop3ds_frame();
    weeds3ds_frame();
    acres3ds_frame();
    settings3ds_frame();
    uint64_t status_start = prof3ds_now();
    pc_platform_swap_buffers();
    prof3ds_add(PROF_STATUS, status_start);
    s_retrace_count++;
    pc_frame_counter++;
    prof3ds_frame();
    trace3ds_mark("game: logic between frames");
}

/* ---- VI ----------------------------------------------------------------------- */

void VIInit(void) {}

void VIConfigure(void* rm)
{
    (void)rm;
}

void VISetNextFrameBuffer(void* fb)
{
    (void)fb;
}

void VISetNextXFB(void* xfb)
{
    (void)xfb;
}

void VIFlush(void) {}

u32 VIGetRetraceCount(void)
{
    return s_retrace_count;
}

void VISetBlack(BOOL black)
{
    (void)black;
}

u32 VIGetTvFormat(void)
{
    return 0; /* VI_NTSC */
}

u32 VIGetDTVStatus(void)
{
    return 0;
}

u32 VIGetCurrentLine(void)
{
    return 0;
}

/* Stored but never called, as on PC. */
void* VISetPreRetraceCallback(void* cb)
{
    void* old = s_pre_retrace_callback;
    s_pre_retrace_callback = cb;
    return old;
}

void* VISetPostRetraceCallback(void* cb)
{
    void* old = s_post_retrace_callback;
    s_post_retrace_callback = cb;
    return old;
}
