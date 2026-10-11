/* test_flags_3ds.c - opt-in testing options (see test_flags_3ds.h). */
#include "pc_platform.h"

#include <stdio.h>

#include "game.h"
#include "m_bgm.h"
#include "m_play.h"
#include "m_player_lib.h"
#include "m_scene.h"

#include "play_3ds.h"
#include "test_flags_3ds.h"

#define NES_TEST_FLAG   "nes_test"
#define NES_DEFAULT_ROM 1
#define NES_MAX_ROM     64
#define BGM_SCENE_EMU   0x11 /* as ac_my_room_msg_ctrl.c_inc does before goto_emu_game */

static int s_nes_rom;       /* 0: nes_test not set */
static bool s_nes_request;

/* NES_DEFAULT_ROM if the file holds no number; 0 if there is no file. */
static int read_nes_rom(void)
{
    FILE* f = fopen(NES_TEST_FLAG, "r");
    if (f == NULL) {
        return 0;
    }
    int rom = NES_DEFAULT_ROM;
    if (fscanf(f, "%d", &rom) != 1 || rom < 1 || rom > NES_MAX_ROM) {
        rom = NES_DEFAULT_ROM;
    }
    fclose(f);
    fprintf(stderr, "[test] %s is set: SELECT+START in town starts NES game %d\n", NES_TEST_FLAG, rom);
    return rom;
}

void test_flags_apply(void)
{
    s_nes_rom = AC3DS_DEBUG ? read_nes_rom() : 0;
}

void test_flags_pad(bool select_held, bool start_pressed)
{
    if (s_nes_rom != 0 && select_held && start_pressed) {
        s_nes_request = true;
    }
}

/* Only from the play scene (town, houses), as the NES furniture does. */
void test_flags_frame(void)
{
    if (!s_nes_request) {
        return;
    }
    s_nes_request = false;
    if (play3ds_current() == NULL) {
        return;
    }
    if (goto_emu_game(gamePT, (s16)s_nes_rom) == TRUE) {
        mBGMPsComp_scene_mode(BGM_SCENE_EMU);
        mPlib_request_main_refuse_type1(gamePT);
        fprintf(stderr, "[test] starting NES game %d\n", s_nes_rom);
    }
}
