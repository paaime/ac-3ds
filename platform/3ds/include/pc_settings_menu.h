/* 3DS version of pc/include/pc_settings_menu.h: stubs, so the menu closes as soon as it opens. */
#ifndef PC_SETTINGS_MENU_H
#define PC_SETTINGS_MENU_H

#include "pc_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

struct game_s;

void pc_settings_menu_enter(void);
int pc_settings_menu_active(void);
int pc_settings_menu_nav_up(void);
int pc_settings_menu_nav_down(void);
int pc_settings_menu_nav_left(void);
int pc_settings_menu_nav_right(void);
int pc_settings_menu_confirm(void);
int pc_settings_menu_cancel(void);
void pc_settings_menu_tick(void);
int pc_settings_menu_capture_active(void);
int pc_settings_menu_handle_capture_event(const void* e);
int pc_settings_menu_capture_blocking(void);
void pc_settings_menu_draw(struct game_s* game, int with_dim_backdrop);

#ifdef __cplusplus
}
#endif

#endif /* PC_SETTINGS_MENU_H */
