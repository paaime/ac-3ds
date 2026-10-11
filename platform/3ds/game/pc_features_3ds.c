/* pc_features_3ds.c - the PC port's optional features (settings file, menus, model viewer), stubbed
 * out: they need SDL and a keyboard. */
#include "pc_platform.h"

#include "pc_pause_menu.h"
#include "pc_settings.h"
#include "pc_settings_menu.h"

/* ---- settings (pc_settings.c defaults) -------------------------------------- */

PCSettings g_pc_settings = {
    .window_width = PC_SCREEN_WIDTH,
    .window_height = PC_SCREEN_HEIGHT,
    .fullscreen = 0,
    .vsync = 0,
    .max_fps = 60,
    .msaa = 0, /* no MSAA on PICA200 */
    .texture_filtering = 1,
    .preload_textures = 0,
    .disable_resetti = 0,
    .disable_shop_visitor_req = 0,
    .borderless_acres = 1,
    .nes_aspect = 1,
    .master_volume = 100,
    .stick_deadzone = 12,
    .cstick_deadzone = 12,
};

void pc_settings_load(void) {}
void pc_settings_save(void) {}
void pc_settings_apply(void) {}

void pc_settings_cycle_resolution(int* width, int* height, int dir)
{
    (void)width;
    (void)height;
    (void)dir;
}

/* ---- title-screen options menu ------------------------------------------------- */

void pc_settings_menu_enter(void) {}
void pc_settings_menu_tick(void) {}

int pc_settings_menu_active(void)
{
    return 0;
}

/* 0 = "the menu is closed", so ac_animal_logo closes it right away. */
int pc_settings_menu_nav_up(void)
{
    return 0;
}

int pc_settings_menu_nav_down(void)
{
    return 0;
}

int pc_settings_menu_nav_left(void)
{
    return 0;
}

int pc_settings_menu_nav_right(void)
{
    return 0;
}

int pc_settings_menu_confirm(void)
{
    return 0;
}

int pc_settings_menu_cancel(void)
{
    return 0;
}

int pc_settings_menu_capture_active(void)
{
    return 0;
}

int pc_settings_menu_handle_capture_event(const void* e)
{
    (void)e;
    return 0;
}

int pc_settings_menu_capture_blocking(void)
{
    return 0;
}

void pc_settings_menu_draw(struct game_s* game, int with_dim_backdrop)
{
    (void)game;
    (void)with_dim_backdrop;
}

/* ---- pause menu ------------------------------------------------------------------ */

int g_pc_paused = 0;
int g_pc_title_main_menu_visible = 0;
int g_pc_nes_active = 0;
int g_pc_pause_input_drain = 0;

void pc_pause_menu_toggle(void) {}

int pc_pause_menu_handle_event(const void* e)
{
    (void)e;
    return 0;
}

void pc_pause_menu_draw(struct game_s* game)
{
    (void)game;
}

/* ---- model viewer ------------------------------------------------------------------- */

int g_pc_model_viewer = 0;
int g_pc_model_viewer_start = 0;
int g_pc_model_viewer_no_cull = 0;

struct game_s;
void pc_model_viewer_init(struct game_s* game)
{
    (void)game;
}

void pc_model_viewer_cleanup(struct game_s* game)
{
    (void)game;
}
