/* town_map_3ds.c - the game's own town map, drawn by the game (see
 * town_map_3ds.h): its grid and acre bubble framed apart, at one scale. */
#include "pc_platform.h"

#include <string.h>

#include "audio.h"
#include "game.h"
#include "graph.h"
#include "libultra/gu.h"
#include "m_common_data.h"
#include "m_event.h"
#include "m_field_info.h"
#include "m_font.h"
#include "m_play.h"
#include "m_player_lib.h"

#include "play_3ds.h"
#include "town_map_3ds.h"

#define ORTHO_NEAR  -800.0f /* as mFont_CulcOrthoMatrix */
#define ORTHO_FAR   800.0f
#define ORTHO_SCALE 1.0f

/* In the menus' units (kan_win_v): the grid's frame (kan_win_futiT_model) and
 * the acre's bubble with its heading (kan_win_wakuT_model). */
static const UnitBox k_grid_box = { -9.0f, 121.0f, -86.0f, 66.0f };
static const UnitBox k_panel_box = { -120.0f, -14.0f, -84.0f, 58.0f };

/* src/game/m_map_ovl.c (TARGET_3DS) */
extern int mMP_3ds_refresh(void);
extern int mMP_3ds_available(void);
extern void mMP_3ds_select(int bx, int bz);
extern void mMP_3ds_draw_grid(GAME* game, Mtx* projection);
extern void mMP_3ds_draw_panel(GAME* game, Mtx* projection);

static bool player_position(GAME_PLAY* play, float* x, float* z, bool* indoors)
{
    *indoors = Common_Get(field_type) != mFI_FIELDTYPE2_FG;
    if (*indoors) {
        if (Common_Get(door_data.next_scene_id) == 0) {
            return false;
        }
        xyz_t pos;
        xyz_t_move_s_xyz(&pos, Common_GetPointer(structure_exit_door_data.exit_position));
        *x = pos.x;
        *z = pos.z;
        return true;
    }
    PLAYER_ACTOR* player = get_player_actor_withoutCheck(play);
    if (player == NULL) {
        return false;
    }
    *x = player->actor_class.world.position.x;
    *z = player->actor_class.world.position.z;
    return true;
}

static bool in_play_scene(void)
{
    return play3ds_current() != NULL && gamePT->graph != NULL && mEv_CheckTitleDemo() <= 0;
}

TownMapState town_map_read(void)
{
    TownMapState s;
    memset(&s, 0, sizeof(s));
    s.available = in_play_scene(); /* not the title demo, not NES games */
    if (s.available) {
        s.player_known = player_position((GAME_PLAY*)gamePT, &s.player_x, &s.player_z, &s.indoors);
    }
    return s;
}

/* mFont_CulcOrthoMatrix's kind of view, showing `box` in `place`. */
static Mtx* framing(GRAPH* graph, UnitBox box, PxRect place)
{
    UnitBox v = bottom_layout_fit(box, place, BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    Mtx* projection = GRAPH_ALLOC_TYPE(graph, Mtx, 1);
    guOrtho(projection, v.left * mFont_SCALE_F, v.right * mFont_SCALE_F, v.bottom * mFont_SCALE_F,
            v.top * mFont_SCALE_F, ORTHO_NEAR, ORTHO_FAR, ORTHO_SCALE);
    return projection;
}

bool town_map_draw(PxRect grid, PxRect panel, int col, int row)
{
    if (!in_play_scene() || !mMP_3ds_refresh()) {
        return false;
    }
    if (col >= 0 && row >= 0) {
        mMP_3ds_select(col, row);
    }
    GRAPH* graph = gamePT->graph;
    mMP_3ds_draw_grid(gamePT, framing(graph, k_grid_box, grid));
    mMP_3ds_draw_panel(gamePT, framing(graph, k_panel_box, panel));
    return true;
}

bool town_map_acre_at(PxRect grid, int x, int y, int* col, int* row)
{
    if (!in_play_scene() || !mMP_3ds_available()) {
        return false;
    }
    UnitBox view = bottom_layout_fit(k_grid_box, grid, BOTTOM_SCREEN_W, BOTTOM_SCREEN_H);
    return bottom_layout_acre_at(view, x, y, TOWN_MAP_COLS, TOWN_MAP_ROWS, col, row);
}

void town_map_cursor_sound(void)
{
    sAdo_SysTrgStart(NA_SE_CURSOL);
}
