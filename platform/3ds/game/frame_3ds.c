/* frame_3ds.c - the citro3d frame lifecycle (see frame_3ds.h). */
#include "frame_3ds.h"

#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>

#include "bottom_3ds.h"
#include "gx_debug.h"
#include "gx_efb.h"
#include "gx_gpu_check.h"
#include "profiler_3ds.h"
#include "settings_3ds.h"
#include "trace_3ds.h"

/* libctru: the linearAlloc pool (heap_3ds.c sets it up) */
extern u32 __ctru_linear_heap;
extern u32 __ctru_linear_heap_size;

#define CMDBUF_BYTES   (1024 * 1024) /* ~800 draws at ~1.2 KB each */
#define TOP_WIDTH      400
#define TOP_HEIGHT     240
#define CLEAR_DEPTH    0 /* citro3d's default depth map: 0 is the far plane */
#define GPU_CHECK_FRAMES 2 /* checked draw by draw with a detailed log */

#define DISPLAY_TRANSFER_FLAGS                                                         \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |                                  \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static struct {
    bool c3d_ready;
    bool gx_ready;
    bool open;
    C3D_RenderTarget* top;
    Frame3dsStats stats;
} F;

/* C3D_Init leaves fragment lighting as the previous program left it (on
 * hardware; Azahar starts at 0), and binding NULL at once is a no-op: binding a
 * throwaway environment first makes the next draw write lighting off. */
static void switch_fragment_lighting_off(void)
{
    static C3D_LightEnv unused;
    C3D_LightEnvInit(&unused);
    C3D_LightEnvBind(&unused);
    C3D_LightEnvBind(NULL);
}

bool frame3ds_init(void)
{
    bool detailed = settings3ds_on(SETTING_VERBOSE_LOG); /* applies at start, as the log itself */
    if (detailed) {
        gxg_log_gpu_state("before C3D_Init (left by the previous program)");
    }
    F.c3d_ready = C3D_Init(CMDBUF_BYTES);
    if (!F.c3d_ready) {
        return false;
    }
    switch_fragment_lighting_off();
    F.top = C3D_RenderTargetCreate(TOP_HEIGHT, TOP_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (F.top == NULL) {
        return false;
    }
    C3D_RenderTargetSetOutput(F.top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    gxe_set_source(F.top->frameBuf.colorBuf, TOP_HEIGHT, TOP_WIDTH); /* portrait, as created */
    fprintf(stderr, "[gx] cache flush per frame: %s\n",
            settings3ds_on(SETTING_FAST_FLUSH) ? "GPU data only (fast flush)" : "the whole GPU pool");
    F.gx_ready = gxr_init();
    if (F.gx_ready && detailed) {
        gxr_gpu_check(GPU_CHECK_FRAMES);
    }
    if (F.gx_ready && !bottom3ds_init()) {
        fprintf(stderr, "[bottom] no town map: its render target failed\n");
    }
    return F.gx_ready;
}

void frame3ds_shutdown(void)
{
    if (F.open) {
        frame3ds_end();
    }
    bottom3ds_shutdown();
    if (F.top != NULL) {
        C3D_RenderTargetDelete(F.top); /* waits for the GPU */
        F.top = NULL;
    }
    if (F.gx_ready) {
        gxr_shutdown();
        F.gx_ready = false;
    }
    if (F.c3d_ready) {
        C3D_Fini();
        F.c3d_ready = false;
    }
}

void frame3ds_begin(void)
{
    if (F.open || !F.gx_ready) {
        return;
    }
    uint64_t wait_start = prof3ds_now();
    trace3ds_mark("frame begin: waiting for the GPU's last frame (C3D_FrameBegin)");
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    prof3ds_add(PROF_FRAME_WAIT, wait_start);
    C3D_RenderTargetClear(F.top, C3D_CLEAR_ALL, gxr_clear_color(), CLEAR_DEPTH);
    C3D_FrameDrawOn(F.top);
    gxr_frame_begin(TOP_WIDTH, TOP_HEIGHT);
    gxr_draw_bars(); /* the 4:3 picture's side bars: first, so never dropped for room */
    F.open = true;
    trace3ds_mark("game: drawing the frame (emu64 -> GX -> citro3d)");
}

void frame3ds_end(void)
{
    if (!F.open) {
        F.stats.idle_waits++;
        trace3ds_mark("frame end: no frame drawn, waiting for VBlank");
        gspWaitForVBlank();
        return;
    }
    F.stats.gx = gxr_stats();
    prof3ds_add_ticks(PROF_TEXTURES, F.stats.gx.texture_ticks);
    uint64_t bottom_start = prof3ds_now();
    trace3ds_mark("frame end: bottom screen");
    bool bottom_drawn = bottom3ds_draw();
    GxrStats all = gxr_stats(); /* with the bottom screen's draws */
    TraceFrameStats ts = { all.draws, all.vertices, all.texture_uploads, all.texture_failures, gxe_copy_count(),
                           all.efb_copy_draws, all.fog_tables, all.cmd_words, all.dropped_draws,
                           all.max_draw_cmd_words, bottom_drawn };
    trace3ds_frame_stats(ts);
    prof3ds_add(PROF_STATUS, bottom_start);
    gxr_frame_end();
    /* What C3D_FrameEnd(0) would do, split out to time it. The fast flush
     * setting flushes only what the CPU wrote for the GPU, as it writes it;
     * Azahar doesn't emulate the caches, so only hardware shows a miss. */
    if (!settings3ds_on(SETTING_FAST_FLUSH)) {
        trace3ds_mark("frame end: cache flush (GSPGPU_FlushDataCache)");
        uint64_t flush_start = prof3ds_now();
        GSPGPU_FlushDataCache((const void*)__ctru_linear_heap, __ctru_linear_heap_size);
        prof3ds_add(PROF_FLUSH, flush_start);
    }
    uint64_t end_start = prof3ds_now();
    trace3ds_mark("frame end: submitting to the GPU (C3D_FrameEnd)");
    C3D_FrameEnd(GX_CMDLIST_FLUSH);
    prof3ds_add(PROF_FRAME_END, end_start);
    gxr_gpu_check_frame_end();
    F.open = false;
    F.stats.frames++;
    F.stats.cpu_ms = C3D_GetProcessingTime();
    F.stats.gpu_ms = C3D_GetDrawingTime();
    F.stats.cmdbuf_usage = C3D_GetCmdBufUsage();
}

bool frame3ds_is_open(void)
{
    return F.open;
}

Frame3dsStats frame3ds_stats(void)
{
    return F.stats;
}
