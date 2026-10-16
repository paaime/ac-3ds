#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "game.h"
#include "graph.h"
#include "libforest/gbi_extensions.h"
#include "m_font.h"
#include "m_rcp.h"
#include "sys_matrix.h"

#include "bottom_dl_3ds.h"
#include "bottom_ui_3ds.h"
#include "gc_pattern.h"
#include "town_map_3ds.h"

#define TEX_DIM          8
#define CHECKER_CELL     4
#define PX_PER_TEXEL     4
#define RECT_SUBPIXELS   4    /* texture rectangles take 10.2 fixed point */
#define TEXEL_STEP_ONE   1024 /* one texel per pixel, 5.10 */
#define ICON_TEXELS      32
#define ICON_TLUT        15
#define ICON_TLUT_SIZE   16
#define DISC_DIM         32
#define ARROW_DIM        16
#define ARROW_PX         20.0f
#define FULL             255
#define LABEL_SCALE_MAX  0.7f
#define TEXT_SCALE       1.0f /* smaller, the 4-bit glyphs alias */
#define NOTE_SCALE       0.75f
#define ROW_TEXT_X       8.0f
#define ROW_LABEL_Y      3.0f
#define ROW_NOTE_Y       20.5f
#define PILL_W           54.0f
#define PILL_H           22.0f
#define PILL_INSET       6.0f
#define PILL_TEXT_MARGIN 6.0f
#define TEXT_GAP         4.0f
#define BORDER           1.0f
#define FONT_LINE_H      16.0f
/* Name plate model (con_kaiwaname_v) extent, menu units from the window centre, y up. */
#define PLATE_LEFT       -112.0f
#define PLATE_RIGHT      -14.0f
#define PLATE_BOTTOM     34.0f
#define PLATE_TOP        62.0f
#define MENU_UNIT        16.0f /* model units per menu unit (mMsg_SetMatrix) */

typedef struct {
    u8 r, g, b, a;
} Rgba;

static const Rgba k_checker_light = { 252, 238, 200, FULL };
static const Rgba k_checker_dark = { 246, 226, 178, FULL };
static const Rgba k_white = { FULL, FULL, FULL, FULL };
static const Rgba k_text = { 92, 60, 36, FULL };
static const Rgba k_note = { 150, 118, 86, FULL };
static const Rgba k_row = { 255, 251, 238, FULL };
static const Rgba k_row_edge = { 214, 176, 124, FULL };
static const Rgba k_pill_on = { 112, 188, 92, FULL };
static const Rgba k_pill_off = { 186, 176, 166, FULL };
static const Rgba k_pill_choice = { 110, 138, 222, FULL };
static const Rgba k_plate = { 70, 245, 255, FULL }; /* a male villager's name plate (m_msg_appear.c_inc) */
static const Rgba k_plate_text = { 0, 0, 15, FULL };

extern void mMsg_3ds_draw_nameplate(GRAPH* graph, u8 r, u8 g, u8 b);

extern u16 inv_mwin_nittki_pal[];
extern u8 inv_mwin_nittki_tex[];
extern u16 inv_mwin_haniwa_pal[];
extern u8 inv_mwin_haniwa_tex[];
extern Gfx inv_item_mode[];

static u8 s_checker[TEX_DIM * TEX_DIM * 2] ATTRIBUTE_ALIGN(32);
static u8 s_disc[DISC_DIM * DISC_DIM * 2] ATTRIBUTE_ALIGN(32);
static u8 s_arrow_up[ARROW_DIM * ARROW_DIM * 2] ATTRIBUTE_ALIGN(32);
static u8 s_arrow_down[ARROW_DIM * ARROW_DIM * 2] ATTRIBUTE_ALIGN(32);

void bottom_ui_init(void)
{
    gc_checker_rgb5a3(s_checker, sizeof(s_checker), TEX_DIM, TEX_DIM, CHECKER_CELL,
                      gc_rgb5a3_opaque(k_checker_light.r, k_checker_light.g, k_checker_light.b),
                      gc_rgb5a3_opaque(k_checker_dark.r, k_checker_dark.g, k_checker_dark.b));
    gc_disc_rgb5a3(s_disc, sizeof(s_disc), DISC_DIM, k_white.r, k_white.g, k_white.b);
    gc_arrow_rgb5a3(s_arrow_up, sizeof(s_arrow_up), ARROW_DIM, true, k_pill_choice.r, k_pill_choice.g,
                    k_pill_choice.b);
    gc_arrow_rgb5a3(s_arrow_down, sizeof(s_arrow_down), ARROW_DIM, false, k_pill_choice.r, k_pill_choice.g,
                    k_pill_choice.b);
}

static int sub(float px)
{
    return (int)(px * RECT_SUBPIXELS);
}

static void screen_view(GRAPH* graph)
{
    Mtx* projection = GRAPH_ALLOC_TYPE(graph, Mtx, 1);
    mFont_CulcOrthoMatrix(projection);
    OPEN_POLY_OPA_DISP(graph);
    gSPMatrix(POLY_OPA_DISP++, projection, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPMatrix(POLY_OPA_DISP++, &Mtx_clear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_POLY_OPA_DISP(graph);
}

static void rect_mode(Gfx** gfx, u32 texture_lut, u32 filter)
{
    gDPPipeSync((*gfx)++);
    gDPSetOtherMode((*gfx)++,
                    G_AD_DISABLE | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | filter | texture_lut | G_TL_TILE |
                        G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PRIM | G_RM_XLU_SURF | G_RM_XLU_SURF2);
}

static void fill(GRAPH* graph, PxRect r, Rgba c)
{
    OPEN_POLY_OPA_DISP(graph);
    Gfx* gfx = POLY_OPA_DISP;
    rect_mode(&gfx, G_TT_NONE, G_TF_POINT);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, c.r, c.g, c.b, c.a);
    gfx = gfx_gSPTextureRectangle1(gfx, sub(r.x), sub(r.y), sub(r.x + r.w), sub(r.y + r.h), G_TX_RENDERTILE, 0, 0, 0, 0);
    POLY_OPA_DISP = gfx;
    CLOSE_POLY_OPA_DISP(graph);
}

static void framed(GRAPH* graph, PxRect r, Rgba edge, Rgba inside)
{
    PxRect in = { r.x + BORDER, r.y + BORDER, r.w - 2 * BORDER, r.h - 2 * BORDER };
    fill(graph, r, edge);
    fill(graph, in, inside);
}

static void checker(GRAPH* graph, PxRect r)
{
    OPEN_POLY_OPA_DISP(graph);
    Gfx* gfx = POLY_OPA_DISP;
    rect_mode(&gfx, G_TT_NONE, G_TF_POINT);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1);
    gDPSetTextureImage_Dolphin(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, TEX_DIM, TEX_DIM, s_checker);
    gDPSetTile_Dolphin(gfx++, G_DOLPHIN_TLUT_DEFAULT_MODE, G_TX_RENDERTILE, 0, GX_REPEAT, GX_REPEAT, GX_CLAMP,
                       GX_CLAMP);
    gfx = gfx_gSPTextureRectangle1(gfx, sub(r.x), sub(r.y), sub(r.x + r.w), sub(r.y + r.h), G_TX_RENDERTILE, 0, 0,
                                   TEXEL_STEP_ONE / PX_PER_TEXEL, TEXEL_STEP_ONE / PX_PER_TEXEL);
    POLY_OPA_DISP = gfx;
    CLOSE_POLY_OPA_DISP(graph);
}

static void soft_texture(GRAPH* graph, PxRect r, u8* texture, int dim)
{
    int step = (int)(TEXEL_STEP_ONE * dim / r.w);
    OPEN_POLY_OPA_DISP(graph);
    Gfx* gfx = POLY_OPA_DISP;
    rect_mode(&gfx, G_TT_NONE, G_TF_BILERP);
    gDPSetCombineLERP(gfx++, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0);
    gDPSetTextureImage_Dolphin(gfx++, G_IM_FMT_RGBA, G_IM_SIZ_16b, dim, dim, texture);
    gDPSetTile_Dolphin(gfx++, G_DOLPHIN_TLUT_DEFAULT_MODE, G_TX_RENDERTILE, 0, GX_CLAMP, GX_CLAMP, GX_CLAMP, GX_CLAMP);
    gfx = gfx_gSPTextureRectangle1(gfx, sub(r.x), sub(r.y), sub(r.x + r.w), sub(r.y + r.h), G_TX_RENDERTILE, 0, 0,
                                   step, step);
    POLY_OPA_DISP = gfx;
    CLOSE_POLY_OPA_DISP(graph);
}

static void disc(GRAPH* graph, PxRect r)
{
    soft_texture(graph, r, s_disc, DISC_DIM);
}

static void icon(GRAPH* graph, PxRect r, u16* palette, u8* texture)
{
    int step = (int)(TEXEL_STEP_ONE * ICON_TEXELS / r.w);
    OPEN_POLY_OPA_DISP(graph);
    Gfx* gfx = POLY_OPA_DISP;
    gSPDisplayList(gfx++, inv_item_mode);
    gDPPipeSync(gfx++);
    gDPSetAlphaCompare(gfx++, G_AC_THRESHOLD);
    gDPSetPrimColor(gfx++, 0, FULL, k_white.r, k_white.g, k_white.b, k_white.a);
    gDPSetBlendColor(gfx++, FULL, FULL, FULL, FULL - 1);
    gDPSetCombineLERP(gfx++, TEXEL0, PRIMITIVE, PRIM_LOD_FRAC, PRIMITIVE, 0, 0, 0, TEXEL0, TEXEL0, PRIMITIVE,
                      PRIM_LOD_FRAC, PRIMITIVE, 0, 0, 0, TEXEL0);
    gDPPipeSync(gfx++);
    gDPSetTextureLUT(gfx++, G_TT_RGBA16);
    gDPSetTexturePersp(gfx++, G_TP_NONE);
    gDPLoadTLUT_Dolphin(gfx++, ICON_TLUT, ICON_TLUT_SIZE, 1, palette);
    gDPSetTextureImage_Dolphin(gfx++, G_IM_FMT_CI, G_IM_SIZ_4b, ICON_TEXELS, ICON_TEXELS, texture);
    gDPSetTile_Dolphin(gfx++, G_DOLPHIN_TLUT_DEFAULT_MODE, G_TX_RENDERTILE, ICON_TLUT, GX_MIRROR, GX_MIRROR, GX_CLAMP,
                       GX_CLAMP);
    gfx = gfx_gSPScisTextureRectangle1(gfx, sub(r.x), sub(r.y), sub(r.x + r.w), sub(r.y + r.h), G_TX_RENDERTILE, 0,
                                       0, step, step);
    gDPSetAlphaCompare(gfx++, G_AC_NONE);
    POLY_OPA_DISP = gfx;
    CLOSE_POLY_OPA_DISP(graph);
}

static float text_width(const char* s, float scale)
{
    return mFont_GetStringWidth((u8*)s, (int)strlen(s), TRUE) * scale;
}

static float fitting(const char* s, float scale, float width)
{
    float w = text_width(s, scale);
    return w > width ? scale * width / w : scale;
}

static void text(GAME* game, const char* s, float x, float y, float scale, Rgba c)
{
    mFont_SetLineStrings(game, (u8*)s, (int)strlen(s), x, y, c.r, c.g, c.b, c.a, FALSE, TRUE, scale, scale,
                         mFont_MODE_POLY);
}

static void text_centred(GAME* game, const char* s, PxRect r, float scale, Rgba c)
{
    float x = r.x + (r.w - text_width(s, scale)) / 2;
    float y = r.y + (r.h - FONT_LINE_H * scale) / 2;
    text(game, s, x, y, scale, c);
}

static void button_label(GAME* game, BottomButton b, const char* label, Rgba c)
{
    PxRect room = bottom_layout_button_label(b);
    float width = text_width(label, 1.0f);
    float scale = width * LABEL_SCALE_MAX > room.w ? room.w / width : LABEL_SCALE_MAX;
    text_centred(game, label, room, scale, c);
}

static void draw_buttons(GAME* game, BottomPage page, bool save_available)
{
    bool options = page == BOTTOM_PAGE_OPTIONS;
    if (options) {
        disc(game->graph, bottom_layout_button_highlight(BOTTOM_BUTTON_OPTIONS));
    }
    icon(game->graph, bottom_layout_button_icon(BOTTOM_BUTTON_OPTIONS), inv_mwin_nittki_pal, inv_mwin_nittki_tex);
    button_label(game, BOTTOM_BUTTON_OPTIONS, options ? "Back" : "Options", k_text);
    if (!options && save_available) {
        icon(game->graph, bottom_layout_button_icon(BOTTOM_BUTTON_SAVE), inv_mwin_haniwa_pal, inv_mwin_haniwa_tex);
        button_label(game, BOTTOM_BUTTON_SAVE, "Save", k_text);
    }
}

static Rgba pill_colour(Settings s, SettingId id)
{
    if (!settings_is_switch(id)) {
        return k_pill_choice;
    }
    return settings_get(s, id) ? k_pill_on : k_pill_off;
}

static void draw_setting(GAME* game, Settings s, SettingId id, PxRect row)
{
    PxRect pill = { row.x + row.w - PILL_W - PILL_INSET, row.y + (row.h - PILL_H) / 2, PILL_W, PILL_H };
    float x = row.x + ROW_TEXT_X;
    float room = pill.x - TEXT_GAP - x;
    framed(game->graph, row, k_row_edge, k_row);
    text(game, settings_label(id), x, row.y + ROW_LABEL_Y, fitting(settings_label(id), TEXT_SCALE, room), k_text);
    text(game, settings_note(id), x, row.y + ROW_NOTE_Y, fitting(settings_note(id), NOTE_SCALE, room), k_note);
    fill(game->graph, pill, pill_colour(s, id));
    const char* value = settings_value_text(s, id);
    text_centred(game, value, pill, fitting(value, TEXT_SCALE, PILL_W - PILL_TEXT_MARGIN), k_white);
}

static void arrow(GRAPH* graph, PxRect r, u8* texture)
{
    PxRect a = { r.x + (r.w - ARROW_PX) / 2, r.y + (r.h - ARROW_PX) / 2, ARROW_PX, ARROW_PX };
    soft_texture(graph, a, texture, ARROW_DIM);
}

/* "1 of 2", not "1/2": the font's '/' is a picture. */
static void draw_pager(GAME* game, int sheet, int sheets)
{
    char label[16];
    if (sheet > 0) {
        arrow(game->graph, bottom_layout_pager_up(), s_arrow_up);
    }
    if (sheet < sheets - 1) {
        arrow(game->graph, bottom_layout_pager_down(), s_arrow_down);
    }
    snprintf(label, sizeof(label), "%d of %d", sheet + 1, sheets);
    PxRect room = bottom_layout_pager_label();
    text_centred(game, label, room, fitting(label, NOTE_SCALE, room.w), k_note);
}

static void name_plate(GAME* game, const char* label, float x, float y)
{
    GRAPH* graph = game->graph;
    float dx = x - BOTTOM_SCREEN_W / 2.0f - PLATE_LEFT;
    float dy = BOTTOM_SCREEN_H / 2.0f - y - PLATE_TOP;
    Matrix_push();
    Matrix_translate(dx * MENU_UNIT, dy * MENU_UNIT, 0.0f, MTX_LOAD);
    OPEN_POLY_OPA_DISP(graph);
    gSPMatrix(POLY_OPA_DISP++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_POLY_OPA_DISP(graph);
    Matrix_pull();
    mMsg_3ds_draw_nameplate(graph, k_plate.r, k_plate.g, k_plate.b);
    OPEN_POLY_OPA_DISP(graph);
    gSPMatrix(POLY_OPA_DISP++, &Mtx_clear, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_POLY_OPA_DISP(graph);
    PxRect plate = { x, y, PLATE_RIGHT - PLATE_LEFT, PLATE_TOP - PLATE_BOTTOM };
    text_centred(game, label, plate, TEXT_SCALE, k_plate_text);
}

static void draw_options(GAME* game, Settings s, SettingsPage page, int sheet)
{
    PxRect title = bottom_layout_title();
    name_plate(game, "Options", title.x, title.y + (title.h - (PLATE_TOP - PLATE_BOTTOM)) / 2);
    int size = settings_page_size(page);
    int sheets = bottom_layout_option_sheets(size);
    bool paged = sheets > 1;
    sheet = sheet < 0 ? 0 : sheet >= sheets ? sheets - 1 : sheet;
    for (int row = 0; row < BOTTOM_LAYOUT_OPTION_ROWS; row++) {
        int n = sheet * BOTTOM_LAYOUT_OPTION_ROWS + row;
        if (n < size) {
            draw_setting(game, s, settings_on_page(page, n), bottom_layout_option_row(row, paged));
        }
    }
    if (paged) {
        draw_pager(game, sheet, sheets);
    }
    int pages = settings_page_count(AC3DS_DEBUG);
    for (int p = 0; pages > 1 && p < pages; p++) {
        PxRect tab = bottom_layout_page_tab(p, pages);
        bool open = p == (int)page;
        if (open) {
            fill(game->graph, tab, k_pill_choice);
        } else {
            framed(game->graph, tab, k_row_edge, k_row);
        }
        text_centred(game, settings_page_name((SettingsPage)p), tab, TEXT_SCALE, open ? k_white : k_text);
    }
}

static void build(GAME* game, const void* context)
{
    BottomUiModel m = *(const BottomUiModel*)context;
    screen_view(game->graph);
    PxRect screen = { 0, 0, BOTTOM_SCREEN_W, BOTTOM_SCREEN_H };
    checker(game->graph, screen);
    draw_buttons(game, m.page, m.save_available); /* before the map: its drawing leaves its own views set */
    if (m.page == BOTTOM_PAGE_OPTIONS) {
        draw_options(game, m.settings, m.options_page, m.options_sheet);
    } else {
        town_map_draw(bottom_layout_grid(), bottom_layout_panel(), m.acre_col, m.acre_row);
    }
}

BottomUiResult bottom_ui_render(BottomUiModel model)
{
    switch (bottom_dl_render(build, &model)) {
        case BOTTOM_DL_DRAWN:
            return BOTTOM_UI_DRAWN;
        case BOTTOM_DL_PARTIAL:
            return BOTTOM_UI_PARTIAL;
        case BOTTOM_DL_NO_GAME:
            return BOTTOM_UI_NOT_YET;
        default:
            return BOTTOM_UI_NONE;
    }
}
