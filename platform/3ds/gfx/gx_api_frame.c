/* GX SDK frame, copy, FIFO and display-list entry points. citro3d owns the
 * command buffer and the frame (game/frame_3ds.c), so most only record state,
 * as in pc/src/pc_gx.c; GXCopyTex copies on the GPU (gx_efb.c). */
#include <dolphin/gx.h>
#include <stdio.h>
#include <string.h>

#include "gx_dlist.h"
#include "gx_efb.h"
#include "gx_render.h"

enum {
    DL_TEXCOPY_SRC = 1,
    DL_TEXCOPY_DST = 2,
    DL_COPY_TEX = 3,
};

static GXDrawDoneCallback s_draw_done_callback;
static GxDlRecorder s_recorder;
static struct {
    u16 src[4]; /* left, top, width, height */
    u16 dst_width, dst_height;
} s_copy;
static bool s_warned_copy_failed;
static bool s_warned_foreign_list;
static bool s_warned_overflow;

/* ---- init and FIFO ------------------------------------------------------- */

GXFifoObj* GXInit(void* base, u32 size)
{
    (void)size;
    return (GXFifoObj*)base;
}

void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size)
{
    (void)fifo;
    (void)base;
    (void)size;
}

void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr)
{
    (void)fifo;
    (void)readPtr;
    (void)writePtr;
}

void GXSaveCPUFifo(GXFifoObj* fifo)
{
    (void)fifo;
}

void GXAbortFrame(void) {}

void GXFlush(void) {}

void GXPixModeSync(void) {}

/* To the game a draw is always done: the callback runs right away. */
void GXSetDrawDone(void)
{
    if (s_draw_done_callback != NULL) {
        s_draw_done_callback();
    }
}

void GXDrawDone(void) {}

GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback cb)
{
    GXDrawDoneCallback old = s_draw_done_callback;
    s_draw_done_callback = cb;
    return old;
}

void GXGetGPStatus(GXBool* overhi, GXBool* underlow, GXBool* readIdle, GXBool* cmdIdle, GXBool* brkpt)
{
    *overhi = GX_FALSE;
    *underlow = GX_FALSE;
    *readIdle = GX_TRUE;
    *cmdIdle = GX_TRUE;
    *brkpt = GX_FALSE;
}

void GXReadXfRasMetric(u32* xf_wait_in, u32* xf_wait_out, u32* ras_busy, u32* clocks)
{
    *xf_wait_in = 0;
    *xf_wait_out = 0;
    *ras_busy = 0;
    *clocks = 0;
}

/* ---- display copy (EFB -> XFB) --------------------------------------------- */

void GXSetCopyClear(GXColor clear_clr, u32 clear_z)
{
    (void)clear_z; /* the depth buffer is always cleared to the far plane */
    gxr_set_clear_color(gxb_rgba(clear_clr.r, clear_clr.g, clear_clr.b, clear_clr.a));
}

void GXSetCopyClamp(GXFBClamp clamp)
{
    (void)clamp;
}

void GXSetCopyFilter(GXBool aa, const u8 sample_pattern[12][2], GXBool vf, const u8 vfilter[7])
{
    (void)aa;
    (void)sample_pattern;
    (void)vf;
    (void)vfilter;
}

void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht)
{
    (void)left;
    (void)top;
    (void)wd;
    (void)ht;
}

void GXSetDispCopyDst(u16 wd, u16 ht)
{
    (void)wd;
    (void)ht;
}

void GXSetDispCopyGamma(GXGamma gamma)
{
    (void)gamma;
}

u32 GXSetDispCopyYScale(f32 vscale)
{
    (void)vscale;
    return 480; /* lines of a 640x480 EFB copy */
}

/* The frame is presented by VIWaitForRetrace (game/vi_3ds.c). */
void GXCopyDisp(void* dest, GXBool clear)
{
    (void)dest;
    (void)clear;
}

/* ---- texture copy (EFB -> texture) ----------------------------------------- */

static bool record(u32 op, const u32* args, u32 count)
{
    if (!gx_dl_recording(&s_recorder)) {
        return false;
    }
    gx_dl_record(&s_recorder, op, args, count);
    return true;
}

void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht)
{
    const u32 args[4] = { left, top, wd, ht };
    if (!record(DL_TEXCOPY_SRC, args, 4)) {
        s_copy.src[0] = left;
        s_copy.src[1] = top;
        s_copy.src[2] = wd;
        s_copy.src[3] = ht;
    }
}

void GXSetTexCopyDst(u16 wd, u16 ht, GXTexFmt fmt, GXBool mipmap)
{
    const u32 args[4] = { wd, ht, (u32)fmt, mipmap };
    if (!record(DL_TEXCOPY_DST, args, 4)) {
        s_copy.dst_width = wd;
        s_copy.dst_height = ht;
    }
}

/* `clear` is ignored: m_play passes false. */
void GXCopyTex(void* dest, GXBool clear)
{
    const u32 args[2] = { (u32)(uintptr_t)dest, clear };
    if (record(DL_COPY_TEX, args, 2)) {
        return;
    }
    if (!gxe_copy(dest, s_copy.src, s_copy.dst_width, s_copy.dst_height) && !s_warned_copy_failed) {
        s_warned_copy_failed = true;
        fprintf(stderr, "[gx] GXCopyTex %ux%u failed\n", s_copy.dst_width, s_copy.dst_height);
    }
}

void GXSetTexCoordScaleManually(GXTexCoordID coord, GXBool enable, u16 ss, u16 ts)
{
    (void)coord;
    (void)enable;
    (void)ss;
    (void)ts;
}

/* ---- display-list recorder -------------------------------------------------- */

void GXBeginDisplayList(void* list, u32 size)
{
    s_recorder = gx_dl_begin(list, size);
}

u32 GXEndDisplayList(void)
{
    bool spoiled = s_recorder.overflow;
    u32 bytes = gx_dl_end(&s_recorder);
    if (spoiled && !s_warned_overflow) {
        s_warned_overflow = true;
        fprintf(stderr, "[gx] GX display list overflowed its %lu-byte buffer; dropped\n",
                (unsigned long)s_recorder.size);
    }
    return bytes;
}

static void replay_command(uint32_t op, const uint32_t* a, uint32_t count, void* ctx)
{
    (void)ctx;
    if (op == DL_TEXCOPY_SRC && count == 4) {
        GXSetTexCopySrc((u16)a[0], (u16)a[1], (u16)a[2], (u16)a[3]);
    } else if (op == DL_TEXCOPY_DST && count == 4) {
        GXSetTexCopyDst((u16)a[0], (u16)a[1], (GXTexFmt)a[2], (GXBool)a[3]);
    } else if (op == DL_COPY_TEX && count == 2) {
        GXCopyTex((void*)(uintptr_t)a[0], (GXBool)a[1]);
    }
}

/* emu64 calls this for the lists m_play recorded (gGXCallDisplayList). */
void GXCallDisplayList(void* list, u32 nbytes)
{
    if (gx_dl_replay(list, nbytes, replay_command, NULL) == 0 && nbytes != 0 && !s_warned_foreign_list) {
        s_warned_foreign_list = true;
        fprintf(stderr, "[gx] GXCallDisplayList: %lu bytes this recorder didn't write; skipped\n",
                (unsigned long)nbytes);
    }
}
