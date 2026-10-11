/* bottom_3ds.c - what the bottom screen shows and when it is redrawn (see
 * bottom_3ds.h): only when something shown changes, else it keeps its picture. */
#include <3ds.h>
#include <citro3d.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "acre_select.h"
#include "bottom_3ds.h"
#include "debounce.h"
#include "bottom_layout.h"
#include "bottom_ui_3ds.h"
#include "gx_render.h"
#include "press_hold.h"
#include "quick_save_3ds.h"
#include "settings_3ds.h"
#include "status_3ds.h"
#include "town_map_3ds.h"
#include "clock_3ds.h"

#define MIN_CMDBUF_WORDS 32768 /* ~400 draws of ~55 words: Options draws each character apart */
#define CLEAR_DEPTH      0
#define COLOR_BG         0xFCEEC8FFu /* 0xRRGGBBAA; all of it is drawn over */
#define REFRESH_FRAMES   1800 /* ~30 s, for names, new villagers, the bridge */
#define MAX_RETRIES      3    /* for a partial picture, then left until a change */
#define STATS_HOLD_MS    2000u
#define NEVER_HELD       UINT32_MAX /* hold threshold: always a tap */
#define ACRE_SHOWN_MS    15000u
#define SAVE_STEADY_MS   500u

#define DISPLAY_TRANSFER_FLAGS                                                         \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |                                  \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

/* What decides a redraw. */
typedef struct {
    BottomView view;
    bool play_scene;        /* the map may show */
    SettingsPage options_page;
    int options_sheet;
    int acre_col, acre_row; /* -1 off the map */
    int chosen_col, chosen_row; /* a tapped acre, -1 for none */
    bool save_available;
    bool indoors;
    Settings settings;
} Shown;

static struct {
    bool ready;
    bool started; /* the boot screen gave way to the map */
    BottomView view;
    SettingsPage options_page; /* kept between visits */
    int options_sheet;
    C3D_RenderTarget* target;
    Shown shown;
    bool dirty;
    uint32_t frames_since_draw;
    uint32_t retries; /* partial redraws in a row */
    bool retrying;
    PressHold corner; /* the Options button */
    PressHold save;
    bool save_shown;
    bool save_fresh;      /* new view: take the state at once */
    Debounce save_change;
    AcreSelect chosen;
    int player_col, player_row; /* -1 off the map */
    aptHookCookie apt_hook;
} B;

/* A press in progress when the HOME Menu or sleep comes is dropped. */
static void on_apt(APT_HookType type, void* param)
{
    (void)param;
    if (type == APTHOOK_ONSUSPEND || type == APTHOOK_ONSLEEP) {
        B.corner = press_hold_idle();
        B.save = press_hold_idle();
    }
}

bool bottom3ds_init(void)
{
    B.target = C3D_RenderTargetCreate(BOTTOM_SCREEN_H, BOTTOM_SCREEN_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (B.target == NULL) {
        return false;
    }
    bottom_ui_init();
    B.view = BOTTOM_DEBUG; /* the boot steps, until the first bottom3ds_input */
    B.chosen = acre_select_none();
    B.player_col = B.player_row = -1;
    aptHook(&B.apt_hook, on_apt, NULL);
    B.ready = true;
    return true;
}

void bottom3ds_shutdown(void)
{
    settings3ds_save(); /* a change made just before quitting */
    if (B.ready) {
        aptUnhook(&B.apt_hook);
    }
    B.ready = false;
    if (B.target != NULL) {
        C3D_RenderTargetDelete(B.target);
        B.target = NULL;
    }
}

/* The render target and the CPU debug console share the bottom framebuffer;
 * only one owns it. Between frames only, with no citro3d frame open. */
static void set_view(BottomView next)
{
    bool was_debug = B.view == BOTTOM_DEBUG;
    B.corner = press_hold_idle();
    B.save = press_hold_idle();
    B.save_fresh = true;
    B.chosen = acre_select_none();
    if (B.view == BOTTOM_OPTIONS && next != BOTTOM_OPTIONS) {
        settings3ds_save(); /* once per visit, not on every tap */
    }
    B.view = next;
    B.dirty = true;
    if (next == BOTTOM_DEBUG && !was_debug) {
        C3D_RenderTargetSetOutput(NULL, GFX_BOTTOM, GFX_LEFT, 0); /* waits for the GPU */
        status3ds_set_visible(true);
    } else if (next != BOTTOM_DEBUG && was_debug) {
        status3ds_set_visible(false);
        C3D_RenderTargetSetOutput(B.target, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    }
}

void bottom3ds_input(void)
{
    if (!B.ready || B.started) {
        return;
    }
    B.started = true;
    set_view(BOTTOM_MAP);
}

void bottom3ds_toggle(void)
{
    if (B.ready) {
        set_view(B.view == BOTTOM_DEBUG ? BOTTOM_MAP : BOTTOM_DEBUG);
    }
}

/* True if the touch hit an arrow. */
static bool touch_pager(int x, int y, int sheets)
{
    BottomPager arrow = bottom_layout_pager_at(x, y);
    if (arrow == BOTTOM_PAGER_UP && B.options_sheet > 0) {
        B.options_sheet--;
        B.dirty = true;
    } else if (arrow == BOTTOM_PAGER_DOWN && B.options_sheet < sheets - 1) {
        B.options_sheet++;
        B.dirty = true;
    }
    return arrow != BOTTOM_PAGER_NONE;
}

static void touch_option(int x, int y)
{
    int pages = settings_page_count(AC3DS_DEBUG);
    int page = pages > 1 ? bottom_layout_page_tab_at(x, y, pages) : -1;
    if (page >= 0) {
        if (page != (int)B.options_page) {
            B.options_sheet = 0;
        }
        B.options_page = (SettingsPage)page;
        B.dirty = true;
        return;
    }
    int size = settings_page_size(B.options_page);
    int sheets = bottom_layout_option_sheets(size);
    bool paged = sheets > 1;
    if (paged && touch_pager(x, y, sheets)) {
        return;
    }
    int first = B.options_sheet * BOTTOM_LAYOUT_OPTION_ROWS;
    int shown = size - first < BOTTOM_LAYOUT_OPTION_ROWS ? size - first : BOTTOM_LAYOUT_OPTION_ROWS;
    int row = bottom_layout_option_at(x, y, shown, paged);
    SettingId id = row >= 0 ? settings_on_page(B.options_page, first + row) : SETTING_COUNT;
    if (id < SETTING_COUNT) {
        settings3ds_toggle(id);
        B.dirty = true;
    }
}

static void touch_map(int x, int y)
{
    int col;
    int row;
    if (!town_map_acre_at(bottom_layout_grid(), x, y, &col, &row)) {
        return;
    }
    int before_col;
    int before_row;
    int after_col;
    int after_row;
    acre_select_shown(B.chosen, B.player_col, B.player_row, &before_col, &before_row);
    B.chosen = acre_select_tap(B.chosen, col, row, B.player_col, B.player_row, clock3ds_ms());
    acre_select_shown(B.chosen, B.player_col, B.player_row, &after_col, &after_row);
    if (after_col != before_col || after_row != before_row) {
        town_map_cursor_sound();
        B.dirty = true;
    }
}

void bottom3ds_touch(int x, int y)
{
    if (!B.ready) {
        return;
    }
    if (B.view == BOTTOM_DEBUG) {
        set_view(BOTTOM_MAP);
        return;
    }
    BottomButton button = bottom_layout_button_at(x, y);
    if (button == BOTTOM_BUTTON_OPTIONS) {
        B.corner = press_hold_begin(clock3ds_ms());
    } else if (button == BOTTOM_BUTTON_SAVE && B.view == BOTTOM_MAP) {
        B.save = press_hold_begin(clock3ds_ms());
    } else if (B.view == BOTTOM_OPTIONS) {
        touch_option(x, y);
    } else {
        touch_map(x, y);
    }
}

/* Options: a tap toggles the page, a hold shows the debug stats (developer build). Save: on release. */
void bottom3ds_touch_held(bool held)
{
    if (!B.ready) {
        return;
    }
    PressStep save = press_hold_step(B.save, held, clock3ds_ms(), NEVER_HELD);
    B.save = save.state;
    if (save.event == PRESS_TAP) {
        quick_save3ds_start(); /* checked again now: does nothing if it can't */
    }
    PressStep step = press_hold_step(B.corner, held, clock3ds_ms(), AC3DS_DEBUG ? STATS_HOLD_MS : NEVER_HELD);
    B.corner = step.state;
    if (step.event == PRESS_TAP) {
        set_view(B.view == BOTTOM_OPTIONS ? BOTTOM_MAP : BOTTOM_OPTIONS);
    } else if (step.event == PRESS_HOLD && status3ds_console_available()) {
        set_view(BOTTOM_DEBUG);
    }
}

/* Follows quick_save3ds_available once steady for SAVE_STEADY_MS, so a tool's
 * swing (START blocked for a moment) doesn't blink the Save button. */
static bool save_looks_available(void)
{
    bool usable = B.view == BOTTOM_MAP && quick_save3ds_available();
    if (usable == B.save_shown || B.save_fresh) {
        B.save_shown = usable;
        B.save_fresh = false;
        B.save_change = debounce_done(B.save_change);
    } else if (!B.save_change.pending) {
        B.save_change = debounce_change(B.save_change, clock3ds_ms());
    } else if (debounce_due(B.save_change, clock3ds_ms(), SAVE_STEADY_MS)) {
        B.save_shown = usable;
        B.save_change = debounce_done(B.save_change);
    }
    return B.save_shown;
}

static Shown current(void)
{
    TownMapState s = town_map_read();
    Shown now;
    memset(&now, 0, sizeof(now));
    now.view = B.view;
    now.play_scene = s.available;
    now.options_page = B.options_page;
    now.options_sheet = B.options_sheet;
    now.acre_col = now.acre_row = -1;
    if (s.available && s.player_known) {
        acre_of_world(s.player_x, s.player_z, TOWN_MAP_BLOCK_WORLD, TOWN_MAP_COLS, TOWN_MAP_ROWS, &now.acre_col,
                      &now.acre_row);
        now.indoors = s.indoors;
    }
    B.player_col = now.acre_col;
    B.player_row = now.acre_row;
    B.chosen = acre_select_step(B.chosen, now.acre_col, now.acre_row, clock3ds_ms(), ACRE_SHOWN_MS);
    now.chosen_col = B.chosen.active ? B.chosen.col : -1;
    now.chosen_row = B.chosen.active ? B.chosen.row : -1;
    now.save_available = save_looks_available();
    now.settings = settings3ds_get();
    return now;
}

static bool same_settings(Settings a, Settings b)
{
    for (int id = 0; id < SETTING_COUNT; id++) {
        if (settings_value(a, (SettingId)id) != settings_value(b, (SettingId)id)) {
            return false;
        }
    }
    return true;
}

/* Field by field: padding bytes needn't survive copies. */
static bool same_shown(Shown a, Shown b)
{
    return a.view == b.view && a.play_scene == b.play_scene && a.options_page == b.options_page &&
           a.options_sheet == b.options_sheet &&
           a.acre_col == b.acre_col && a.acre_row == b.acre_row && a.chosen_col == b.chosen_col &&
           a.chosen_row == b.chosen_row && a.save_available == b.save_available && a.indoors == b.indoors &&
           same_settings(a.settings, b.settings);
}

static void render(Shown now)
{
    C3D_RenderTargetClear(B.target, C3D_CLEAR_ALL, COLOR_BG, CLEAR_DEPTH);
    C3D_FrameDrawOn(B.target);
    gxr_set_target(BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    BottomUiModel model = { now.view == BOTTOM_OPTIONS ? BOTTOM_PAGE_OPTIONS : BOTTOM_PAGE_MAP, now.settings,
                            now.options_page, now.options_sheet, now.chosen_col, now.chosen_row,
                            now.save_available };
    B.retries = B.retrying ? B.retries : 0;
    B.retrying = false;
    BottomUiResult result = bottom_ui_render(model);
    if (result == BOTTOM_UI_NOT_YET) {
        B.dirty = true; /* no game yet: every frame */
    } else if (result == BOTTOM_UI_PARTIAL && B.retries < MAX_RETRIES) {
        B.retries++;
        B.retrying = true;
        B.dirty = true; /* short of draws or textures */
    }
}

bool bottom3ds_draw(void)
{
    if (!B.ready || B.view == BOTTOM_DEBUG) {
        return false;
    }
    Shown now = current();
    B.frames_since_draw++;
    bool stale = B.frames_since_draw >= REFRESH_FRAMES;
    if (!B.dirty && !stale && same_shown(now, B.shown)) {
        return false;
    }
    if (gpuCmdBufSize - gpuCmdBufOffset < MIN_CMDBUF_WORDS) {
        return false;
    }
    B.shown = now;
    B.dirty = false;
    B.frames_since_draw = 0;
    render(now);
    return true;
}
