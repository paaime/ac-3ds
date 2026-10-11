/* bottom_dl_3ds.c - the bottom screen's own display list (see bottom_dl_3ds.h):
 * the polygon arena is swapped for a private buffer while the list is built,
 * after the frame's own emu64 pass, so nothing else is using either. */
#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "game.h"
#include "graph.h"
#include "libforest/emu64/emu64_wrapper.h"
#include "main.h"
#include "m_font.h"
#include "m_rcp.h"
#include "sys_ucode.h"

#include "bottom_dl_3ds.h"
#include "gx_render.h"

#define DL_GFX      0x4000 /* 128 KB: the map and its bar use a few thousand commands */
#define EFB_RIGHT   640
#define EFB_BOTTOM  480
#define UCODE_COUNT 2
#define VP_DEPTH    511 /* as initView's set_viewport (m_view.c) */
#define VP_UNITS    4

static Gfx s_dl[DL_GFX];
static bool s_warned_overflow;

/* Menus inherit the viewport from the frame's earlier lists; this list has none.
 * In the arena, as mSM_setup_view does, so seg2k0 can't take a heap address. */
static Vp* screen_viewport(GRAPH* graph)
{
    Vp* vp = GRAPH_ALLOC_TYPE(graph, Vp, 1);
    vp->vp.vscale[0] = vp->vp.vtrans[0] = SCREEN_WIDTH * VP_UNITS;
    vp->vp.vscale[1] = vp->vp.vtrans[1] = SCREEN_HEIGHT * VP_UNITS;
    vp->vp.vscale[2] = vp->vp.vtrans[2] = VP_DEPTH;
    vp->vp.vscale[3] = vp->vp.vtrans[3] = 0;
    return vp;
}

/* As DisplayList_initialize, plus the viewport and the font in polygon mode. */
static void open_list(GRAPH* graph)
{
    Vp* viewport = screen_viewport(graph);
    OPEN_DISP(graph);
    gSPDisplayList(NOW_POLY_OPA_DISP++, RSP_RDP_clear_data);
    gDPSetScissor(NOW_POLY_OPA_DISP++, G_SC_NON_INTERLACE, 0, 0, EFB_RIGHT, EFB_BOTTOM);
    gSPViewport(NOW_POLY_OPA_DISP++, viewport);
    CLOSE_DISP(graph);
    mFont_SetMode(graph, mFont_MODE_POLY);
}

static void close_list(GRAPH* graph)
{
    OPEN_DISP(graph);
    gDPPipeSync(NOW_POLY_OPA_DISP++);
    gDPFullSync(NOW_POLY_OPA_DISP++);
    gSPEndDisplayList(NOW_POLY_OPA_DISP++);
    CLOSE_DISP(graph);
}

/* As graph_task_set00 runs a frame's lists. */
static void run_emu64(Gfx* list)
{
    ucode_info ucode[UCODE_COUNT];
    ucode[0].type = UCODE_TYPE_POLY_TEXT;
    ucode[0].ucode_p = ucode_GetPolyTextStart();
    ucode[1].type = UCODE_TYPE_SPRITE_TEXT;
    ucode[1].ucode_p = ucode_GetSpriteTextStart();
    emu64_init();
    emu64_set_ucode_info(UCODE_COUNT, ucode);
    emu64_set_first_ucode(ucode[0].ucode_p);
    emu64_taskstart(list);
    emu64_cleanup();
}

BottomDlResult bottom_dl_render(BottomDlBuild build, const void* context)
{
    if (gamePT == NULL || gamePT->graph == NULL) {
        return BOTTOM_DL_NO_GAME;
    }
    if (build == NULL) {
        return BOTTOM_DL_NONE;
    }
    GRAPH* graph = gamePT->graph;
    THA_GA saved = graph->polygon_opaque_thaga;
    THA_GA_ct(&graph->polygon_opaque_thaga, s_dl, sizeof(s_dl));
    open_list(graph);
    build(gamePT, context);
    close_list(graph);
    bool overflowed = THA_GA_isCrash(&graph->polygon_opaque_thaga);
    graph->polygon_opaque_thaga = saved;
    if (overflowed) {
        if (!s_warned_overflow) {
            s_warned_overflow = true;
            fprintf(stderr, "[bottom] the bottom screen's display list overflowed its %u bytes\n",
                    (unsigned)sizeof(s_dl));
        }
        return BOTTOM_DL_NONE;
    }
    GxrStats before = gxr_stats();
    run_emu64(s_dl);
    GxrStats after = gxr_stats();
    bool partial = after.dropped_draws != before.dropped_draws || after.texture_failures != before.texture_failures ||
                   after.vertex_overflow != before.vertex_overflow;
    return partial ? BOTTOM_DL_PARTIAL : BOTTOM_DL_DRAWN;
}
