/* quick_save_3ds.c - the bottom screen's Save button (see quick_save_3ds.h).
 * Never saves the town in place: only leaving it puts what its actors hold back
 * in the town's data (restore_fgdata_all). So the button makes the gyroid's scene
 * change to the save room, which sends the player back to where they stood
 * (ac_npc_restart_schedule.c_inc, TARGET_3DS). */
#include "pc_platform.h"

#include <string.h>

#include "game.h"
#include "m_bgm.h"
#include "m_common_data.h"
#include "m_event.h"
#include "m_field_info.h"
#include "m_house.h"
#include "m_npc.h"
#include "m_play.h"
#include "m_player_lib.h"
#include "m_private.h"
#include "m_scene.h"
#include "m_submenu.h"

#include "play_3ds.h"
#include "quick_save.h"
#include "quick_save_3ds.h"

#define SAVE_BGM_MODE 13 /* the gyroid's "save" choice (ac_haniwa_move.c_inc) */
#define SAVE_WIPE     3  /* the house door's (ac_my_house_move.c_inc) */
#define DOOR_BGM_FADE 0x249 /* the house door's music fade (ac_my_house_move.c_inc) */

static struct {
    bool pending;
    s16 x, y, z;
    u8 direction;
} s_return;

/* The player's own house, as the gyroid checks (aHNW_set_save_permission). */
static mHm_hs_c* own_house(void)
{
    u32 player_no = Common_Get(player_no);
    if (player_no >= PLAYER_NUM) {
        return NULL; /* a visitor */
    }
    mHm_hs_c* house = Save_GetPointer(homes[mHS_get_arrange_idx(player_no)]);
    PersonalID_c* pid = &Save_Get(private_data[player_no]).player_ID;
    return mPr_NullCheckPersonalID(pid) != TRUE && mPr_CheckCmpPersonalID(pid, &house->ownerID) == TRUE ? house
                                                                                                         : NULL;
}

static QuickSaveCheck read_check(GAME_PLAY* play)
{
    QuickSaveCheck c;
    memset(&c, 0, sizeof(c));
    PLAYER_ACTOR* player = get_player_actor_withoutCheck(play);
    mHm_hs_c* house = own_house();
    c.in_town_outdoors = player != NULL && Save_Get(scene_no) == SCENE_FG &&
                         Common_Get(field_type) == mFI_FIELDTYPE2_FG;
    if (c.in_town_outdoors) {
        int bx;
        int bz;
        mFI_Wpos2BlockNum(&bx, &bz, player->actor_class.world.position);
        c.on_island = mFI_CheckBlockKind_OR(bx, bz, mRF_BLOCKKIND_ISLAND) != FALSE;
    }
    c.first_intro = mEv_CheckFirstIntro() != FALSE;
    c.title_demo = mEv_CheckTitleDemo() > 0;
    c.fading = play->fb_wipe_mode != WIPE_MODE_NONE || play->fb_fade_type != FADE_TYPE_NONE; /* as START */
    c.reset_pending = Common_Get(reset_flag) != FALSE;
    c.menu_open = play->submenu.process_status != mSM_PROCESS_WAIT || play->submenu.start_refuse ||
                  play->submenu.start_refuse_timer != 0;
    c.player_free = c.in_town_outdoors && mPlib_able_submenu_type1((GAME*)play);
    c.house_owner = house != NULL;
    c.needs_a_friend = house != NULL && house->flags.has_saved == FALSE && mEv_CheckFirstJob() == TRUE &&
                       mNpc_GetFriendAnimalNum(&Common_Get(now_private)->player_ID) == 0;
    return c;
}

bool quick_save3ds_available(void)
{
    GAME_PLAY* play = play3ds_current();
    return play != NULL && quick_save_allowed(read_check(play));
}

bool quick_save3ds_start(void)
{
    // clang-format off
    static Door_data_c save_room = { /* the house door's (ac_my_house_move.c_inc) */
        SCENE_PLAYERSELECT_SAVE, mSc_DIRECT_NORTH, 0, 0, { 100, 0, 120 }, EMPTY_NO, 1, { 0, 0, 0 }
    };
    // clang-format on
    GAME_PLAY* play = play3ds_current();
    if (play == NULL || !quick_save_allowed(read_check(play))) {
        return false;
    }
    PLAYER_ACTOR* player = get_player_actor_withoutCheck(play);
    xyz_t pos = player->actor_class.world.position;
    if (goto_other_scene(play, &save_room, TRUE) != 1) {
        return false;
    }
    mBGMPsComp_scene_mode(SAVE_BGM_MODE);
    mBGMPsComp_make_ps_wipe(DOOR_BGM_FADE); /* without it the scene change waits ~2 s for the music */
    Common_Get(transition).wipe_type = SAVE_WIPE;
    play->fb_fade_type = FADE_TYPE_DEMO;
    own_house()->flags.has_saved = TRUE; /* as the gyroid (aHNW_set_save_permission) */
    s_return.pending = true;
    s_return.x = (s16)pos.x;
    s_return.y = (s16)pos.y;
    s_return.z = (s16)pos.z;
    s_return.direction = (u8)quick_save_direction((u16)player->actor_class.shape_info.rotation.y);
    return true;
}

/* Asked by the save room's "continue" (ac_npc_restart_schedule.c_inc). */
int ac3ds_save_return(short* x, short* y, short* z, unsigned char* direction)
{
    if (!s_return.pending) {
        return FALSE;
    }
    s_return.pending = false;
    *x = s_return.x;
    *y = s_return.y;
    *z = s_return.z;
    *direction = s_return.direction;
    return TRUE;
}

void ac3ds_save_return_clear(void)
{
    s_return.pending = false;
}
