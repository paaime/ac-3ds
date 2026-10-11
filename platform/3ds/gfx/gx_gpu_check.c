/* gx_gpu_check.c - the draw-by-draw GPU check (see gx_gpu_check.h). */
#include "gx_gpu_check.h"

#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>

/* citro3d 1.7.1's context (source/internal.h, C3D_Context) starts with the GX
 * command queue that C3D_FrameBegin waits on. */
extern unsigned char __C3D_Context[];

#define GPU_CHECK_TIMEOUT_NS 2000000000LL /* any draw takes far less */

static uint32_t s_frames; /* drawing frames still to check */

static gxCmdQueue_s* c3d_queue(void)
{
    return (gxCmdQueue_s*)(void*)__C3D_Context;
}

void gxg_arm(uint32_t frames)
{
    s_frames = frames;
}

bool gxg_active(void)
{
    return s_frames > 0;
}

bool gxg_run_and_wait(void)
{
    gxCmdQueue_s* queue = c3d_queue();
    gxCmdQueueRun(queue);
    return gxCmdQueueWait(queue, GPU_CHECK_TIMEOUT_NS);
}

void gxg_empty_queue(void)
{
    gxCmdQueueStop(c3d_queue());
    gxCmdQueueClear(c3d_queue());
}

void gxg_draw_failed(uint32_t draw)
{
    fprintf(stderr, "[gpu] the GPU didn't finish draw #%lu within 2 s: it stopped on the [gxdump] line above\n",
            (unsigned long)draw);
    s_frames = 0;
}

bool gxg_frame_end(uint32_t draws)
{
    if (s_frames == 0) {
        return true;
    }
    bool finished = gxg_run_and_wait();
    if (!finished) {
        fprintf(stderr, "[gpu] the GPU finished the draws, not the frame's end (screen transfers), within 2 s\n");
        s_frames = 0;
    } else if (draws > 0) {
        fprintf(stderr, "[gpu] checked: the GPU finished this frame's %lu draws one by one, and its end\n",
                (unsigned long)draws);
        s_frames--;
    }
    return finished;
}

/* Read through GSP: the GPU's internal registers are at 0x401000 in its
 * register space, 4 bytes each. */
void gxg_log_gpu_state(const char* when)
{
    static const struct {
        u16 reg;
        const char* name;
    } k_regs[] = {
        { 0x08F, "LIGHTING_ENABLE0" }, { 0x1C6, "LIGHTING_ENABLE1" }, { 0x1C3, "LIGHTING_CONFIG0" },
        { 0x1C4, "LIGHTING_CONFIG1" }, { 0x1C2, "LIGHTING_NUM_LIGHTS" }, { 0x047, "CLIPPLANE_ENABLE" },
        { 0x080, "TEXUNIT_CONFIG" }, { 0x0A8, "PROCTEX_CONFIG" }, { 0x0E0, "TEXENV_UPDATE_BUFFER" },
        { 0x22D, "POST_VERTEX_CACHE_NUM" }, { 0x064, "SH_OUTATTR_MODE" }, { 0x06F, "SH_OUTATTR_CLOCK" },
    };
    fprintf(stderr, "[gpustate] %s:", when);
    for (size_t i = 0; i < sizeof(k_regs) / sizeof(k_regs[0]); i++) {
        u32 value = 0;
        Result r = GSPGPU_ReadHWRegs(0x401000 + k_regs[i].reg * 4u, &value, sizeof(value));
        if (R_FAILED(r)) {
            fprintf(stderr, " %s unreadable (%08lX);", k_regs[i].name, (unsigned long)r);
        } else {
            fprintf(stderr, " %s %08lX;", k_regs[i].name, (unsigned long)value);
        }
    }
    fprintf(stderr, "\n");
}
