/* pc_pause_menu.h — 3DS version of pc/include/pc_pause_menu.h without SDL
 * types; no-ops on 3DS (game/pc_features_3ds.c). */
#ifndef PC_PAUSE_MENU_H
#define PC_PAUSE_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

struct game_s; /* matches typedef in game_h.h */

extern int g_pc_paused;
extern int g_pc_title_main_menu_visible; /* set by ac_animal_logo */
extern int g_pc_nes_active;

void pc_pause_menu_toggle(void);
int pc_pause_menu_handle_event(const void* e);
void pc_pause_menu_draw(struct game_s* game);

#ifdef __cplusplus
}
#endif

#endif /* PC_PAUSE_MENU_H */
