#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "m_camera2.h"
#include "m_common_data.h"
#include "m_demo.h"
#include "m_event.h"
#include "m_font.h"
#include "m_msg.h"
#include "m_msg_enum.h"
#include "m_player_lib.h"
#include "m_private.h"

#include "look.h"
#include "look_3ds.h"
#include "look_talk.h"
#include "play_3ds.h"
#include "port_msg.h"

/* A spare byte of the player's own save data that no game code reads or writes. */
#define TAN_FLOOR_BYTE(p) ((p)->unused_2412[0])

#define GYROID_ANYTHING_ELSE MSG_2343
/* Rover's two replies to the money question: each one's second page starts the arrival. */
#define ROVER_MONEY_YES      0x2AD2
#define ROVER_MONEY_NO       0x2AD4
#define CODE_CLEAR_LEN       2
#define PREVIEW_DISTANCE     260.0f
#define PREVIEW_HEIGHT       20.0f
#define PREVIEW_TILT         ((s16)0x9000)
#define PREVIEW_FRONT        ((s16)0x8000)
#define PREVIEW_SIDE         ((s16)0x2000)
#define PREVIEW_MORPH        20
#define PREVIEW_PRIORITY     (mDemo_CAMERA_PRIORITY + 1)
#define RETURN_PRIORITY      (mDemo_CAMERA_PRIORITY + 2)

_Static_assert(LOOK_FACES == mPr_FACE_TYPE_NUM, "look.h's faces are the game's");
_Static_assert(LOOK_TONES == mPlayer_SUNBURN_MAX_RANK + 1, "look.h's tones are the game's tan ranks");

static struct {
    int face;
    int rank;
    u8 floor_byte;
    bool tone_changed;
    bool at_limit;
    bool talking;
    size_t resume_len;
    u8 resume[mMsg_MSG_BUF_SIZE];
} s_look;

int ac3ds_look_tan_floor(void)
{
    return Now_Private != NULL ? look_floor_from_save(TAN_FLOOR_BYTE(Now_Private)) : 0;
}

static PLAYER_ACTOR* player_actor(void)
{
    GAME_PLAY* play = play3ds_current();
    return play != NULL ? get_player_actor_withoutCheck(play) : NULL;
}

static void camera_on_face(void)
{
    PLAYER_ACTOR* player = player_actor();
    if (player == NULL) {
        return;
    }
    s16 facing = player->actor_class.shape_info.rotation.y;
    s_xyz angle = { PREVIEW_TILT, (s16)(facing + PREVIEW_FRONT + PREVIEW_SIDE), 0 };
    xyz_t center = player->actor_class.world.position;
    center.y += PREVIEW_HEIGHT;
    Camera2_request_main_simple(play3ds_current(), &center, &angle, PREVIEW_DISTANCE, PREVIEW_MORPH, PREVIEW_PRIORITY);
}

static void camera_back(void)
{
    GAME_PLAY* play = play3ds_current();
    PLAYER_ACTOR* player = player_actor();
    ACTOR* speaker = mDemo_Get_talk_actor();
    if (player != NULL && speaker != NULL) {
        Camera2_request_main_talk(play, speaker, (ACTOR*)player, RETURN_PRIORITY);
    } else if (play != NULL) {
        Camera2_change_priority(play, 0);
    }
}

static void redraw_face(void)
{
    if (player_actor() != NULL) {
        mPlib_change_player_face((GAME*)play3ds_current());
    }
}

static void start(void)
{
    s_look.talking = true;
    s_look.face = Now_Private->face;
    s_look.rank = Now_Private->sunburn.rank;
    s_look.floor_byte = TAN_FLOOR_BYTE(Now_Private);
    s_look.tone_changed = false;
    s_look.at_limit = false;
    camera_on_face();
}

static void set_tone(int tone)
{
    s_look.at_limit = tone == Now_Private->sunburn.rank;
    Now_Private->sunburn.rank = (s8)tone;
    s_look.tone_changed = true;
}

static void finish(void)
{
    if (s_look.tone_changed) {
        TAN_FLOOR_BYTE(Now_Private) = look_floor_to_save(Now_Private->sunburn.rank);
    }
    s_look.talking = false;
    fprintf(stderr, "[look] face %d, tone %d, tan floor %d\n", Now_Private->face, Now_Private->sunburn.rank,
            ac3ds_look_tan_floor());
    camera_back();
}

/* The game only fades a tan that's there: a save whose tan faded to nothing elsewhere comes back to the floor. */
void look3ds_frame(void)
{
    PLAYER_ACTOR* player = player_actor();
    if (s_look.talking || player == NULL || Now_Private == NULL || !mEv_IsNotTitleDemo()) {
        return;
    }
    int floor = ac3ds_look_tan_floor();
    if (Now_Private->sunburn.rank < floor) {
        Now_Private->sunburn.rank = (s8)floor;
        mPlib_change_player_face_pallet((GAME*)play3ds_current());
    }
}

static void set_tone_by_name(int tone)
{
    Now_Private->sunburn.rank = (s8)tone;
    TAN_FLOOR_BYTE(Now_Private) = look_floor_to_save(tone);
    fprintf(stderr, "[look] tone %d (%s) on the train\n", tone, look_talk_tone_name(tone));
}

static void apply(LookTalkStep step)
{
    switch (step.effect) {
    case LOOK_TALK_START:
        start();
        return;
    case LOOK_TALK_SET_TONE:
        set_tone_by_name(step.tone);
        return;
    case LOOK_TALK_FACE:
        Now_Private->face = (s8)look_face_step(Now_Private->face, step.delta);
        break;
    case LOOK_TALK_FACE_BACK:
        Now_Private->face = (s8)s_look.face;
        break;
    case LOOK_TALK_TONE:
        set_tone(look_tone_step(Now_Private->sunburn.rank, step.delta));
        break;
    case LOOK_TALK_TONE_BACK:
        Now_Private->sunburn.rank = (s8)s_look.rank;
        TAN_FLOOR_BYTE(Now_Private) = s_look.floor_byte;
        s_look.tone_changed = false;
        break;
    case LOOK_TALK_FINISH:
        finish();
        return;
    default:
        return;
    }
    redraw_face();
}

GcMsgChoice look3ds_menu_choice(int index)
{
    static const int k_gyroid_menus[] = { MSG_2341, MSG_2343, MSG_2352, MSG_2355, MSG_2358 };
    GcMsgChoice none = { 0, 0 };
    for (size_t i = 0; i < sizeof(k_gyroid_menus) / sizeof(k_gyroid_menus[0]); i++) {
        if (index == k_gyroid_menus[i]) {
            GcMsgChoice change = { LOOK_TALK_STRING_CHANGE, (uint16_t)look_talk_start(LOOK_TALK_GYROID) };
            return change;
        }
    }
    return none;
}

static int code_size(const uint8_t* code)
{
    return mFont_CodeSize_get((u8*)code);
}

bool look3ds_patch_rover(int index, unsigned char* buf, int len, int size)
{
    if ((index != ROVER_MONEY_YES && index != ROVER_MONEY_NO) || len <= 0 || size <= 0) {
        return false;
    }
    size_t cut = gc_msg_first_page_break(buf, (size_t)len, code_size);
    size_t tail = cut != 0 && (size_t)len > cut + CODE_CLEAR_LEN ? (size_t)len - cut - CODE_CLEAR_LEN : 0;
    u8 head[mMsg_MSG_BUF_SIZE];
    size_t head_len = cut != 0 ? gc_msg_cut_to(buf, cut, head, sizeof(head),
                                               (uint16_t)look_talk_start(LOOK_TALK_ROVER))
                               : 0;
    if (head_len == 0 || tail == 0 || head_len > (size_t)size || tail > sizeof(s_look.resume)) {
        fprintf(stderr, "[look] Rover's message %d left as it is\n", index);
        return false;
    }
    memcpy(s_look.resume, buf + cut + CODE_CLEAR_LEN, tail);
    s_look.resume_len = tail;
    memcpy(buf, head, head_len);
    return true;
}

static bool build_resume(unsigned char* buf, int size)
{
    if (s_look.resume_len == 0 || s_look.resume_len > (size_t)size) {
        fprintf(stderr, "[look] nothing to resume Rover's talk with\n");
        return false;
    }
    memcpy(buf, s_look.resume, s_look.resume_len);
    return true;
}

bool look3ds_msg_build(int index, unsigned char* buf, int size)
{
    if (index == PORT_MSG_RESUME) {
        return build_resume(buf, size);
    }
    if (!look_talk_owns(index)) {
        return false;
    }
    if (Now_Private != NULL) {
        apply(look_talk_step(index));
    }
    LookTalkState st = { s_look.at_limit, Now_Private != NULL ? Now_Private->sunburn.rank : 0, GYROID_ANYTHING_ELSE };
    size_t len = size > 0 ? look_talk_build(index, st, buf, (size_t)size) : 0;
    if (len == 0) {
        fprintf(stderr, "[look] message %d couldn't be built\n", index);
    }
    return len != 0;
}
