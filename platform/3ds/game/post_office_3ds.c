#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "m_common_data.h"
#include "m_font.h"
#include "m_land.h"
#include "m_msg.h"
#include "m_private.h"

#include "gc_msg.h"
#include "ordinance_3ds.h"
#include "ordinance_talk.h"
#include "post_office_3ds.h"

#define ORDINANCES_CHOICE 3

_Static_assert(ORDINANCE_TALK_MSG_FIRST == AC3DS_MSG_FIRST, "the port's messages start where the game's end");
_Static_assert(ORDINANCE_TALK_MSG_FIRST + ORDINANCE_TALK_MSG_COUNT <= AC3DS_MSG_END, "room for the port's messages");
_Static_assert(ORDINANCE_TALK_STRING_ORDINANCES == mChoice_SELECT_STR_NUM, "strings start where the game's end");
_Static_assert(ORDINANCE_TALK_STRING_LEN == mChoice_CHOICE_STRING_LEN, "a choice string's length");
_Static_assert((int)GC_MSG_FORCE_NEXT == (int)mFont_CONT_CODE_SET_FORCE_NEXT, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SOUND_CUT == (int)mFont_CONT_CODE_SOUND_CUT, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SELECT_NO_B == (int)mFont_CONT_CODE_SELECT_NO_B, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SELECT_STRING_6 == (int)mFont_CONT_CODE_SET_SELECT_STRING_6, "gc_msg's control codes are m_font's");
_Static_assert(GC_MSG_NEW_LINE == CHAR_NEW_LINE, "gc_msg's new line is m_font's");
_Static_assert(GC_MSG_TAG == CHAR_MESSAGE_TAG, "gc_msg's message tag is m_font's");

/* Pelly's menus: aPG_set_talk_info's greetings, then aPG_repay_after_init's. Phyllis's follow each. */
static const int k_pelly_menus[] = {
    0x31d3, 0x31d1, 0x08b1, 0x08ad, 0x08d1, 0x08cf, 0x31d7, 0x31d5, 0x08db, 0x08d9, 0x08df, 0x08dd,
};

static struct {
    int patched_menu;
    u8 patched[mMsg_MSG_BUF_SIZE];
} s_post = { -1, { 0 } };

static int menu_speaker(int index)
{
    for (size_t i = 0; i < sizeof(k_pelly_menus) / sizeof(k_pelly_menus[0]); i++) {
        if (index == k_pelly_menus[i]) {
            return ORDINANCE_TALK_PELLY;
        }
        if (index == k_pelly_menus[i] + 1) {
            return ORDINANCE_TALK_PHYLLIS;
        }
    }
    return -1;
}

static int code_size(const uint8_t* code)
{
    return mFont_CodeSize_get((u8*)code);
}

static OrdinanceTalkState talk_state(void)
{
    OrdinanceTalkState st = { ordinance3ds_get(), Now_Private != NULL ? Now_Private->inventory.wallet : 0 };
    return st;
}

int ac3ds_msg_build(int index, unsigned char* buf, int size)
{
    size_t len = size > 0 ? ordinance_talk_build(index, talk_state(), buf, (size_t)size) : 0;
    if (len == 0) {
        fprintf(stderr, "[ordinance] message %d couldn't be built\n", index);
    }
    return len != 0;
}

void ac3ds_msg_patch(int index, unsigned char* buf, int len, int size)
{
    if (ordinance3ds_sign(index, buf, size)) {
        return;
    }
    int speaker = menu_speaker(index);
    if (speaker < 0) {
        return;
    }
    if (mLd_PlayerManKindCheck() != NATIVE) {
        s_post.patched_menu = -1;
        return;
    }
    GcMsgChoice ordinances = { ORDINANCE_TALK_STRING_ORDINANCES,
                               (uint16_t)ordinance_talk_intro((OrdinanceTalkSpeaker)speaker) };
    size_t room = size < (int)sizeof(s_post.patched) ? (size_t)size : sizeof(s_post.patched);
    size_t n = len > 0 ? gc_msg_insert_choice(buf, (size_t)len, s_post.patched, room, ordinances, code_size) : 0;
    s_post.patched_menu = n != 0 ? index : -1;
    if (n == 0) {
        fprintf(stderr, "[ordinance] the post office menu %d has no room for Ordinances\n", index);
        return;
    }
    memcpy(buf, s_post.patched, n);
}

int ac3ds_post_menu_choice(int choice)
{
    if (s_post.patched_menu < 0 || mMsg_Get_msg_num(mMsg_Get_base_window_p()) != s_post.patched_menu) {
        return choice;
    }
    if (choice == ORDINANCES_CHOICE) {
        return AC3DS_POST_CHOICE_ORDINANCES;
    }
    return choice > ORDINANCES_CHOICE ? choice - 1 : choice;
}

static void enact(Ordinance o)
{
    if (Now_Private == NULL || !ordinance_affordable(Now_Private->inventory.wallet)) {
        fprintf(stderr, "[ordinance] %s not enacted: the wallet can't pay\n", ordinance_name(o));
        return;
    }
    Now_Private->inventory.wallet = ordinance_pay(Now_Private->inventory.wallet);
    ordinance3ds_set(o);
}

int ac3ds_post_ordinance_talk(void)
{
    OrdinanceTalkStep step = ordinance_talk_step(mMsg_Get_msg_num(mMsg_Get_base_window_p()));
    if (step.effect == ORDINANCE_TALK_ENACT) {
        enact(step.ordinance);
    } else if (step.effect == ORDINANCE_TALK_REPEAL) {
        ordinance3ds_set(ORDINANCE_NONE);
    }
    return step.back_to_menu;
}

void ac3ds_choice_load_string(mChoice_c* choice, u8* str, int str_no, ACTOR* actor)
{
    const char* text = ordinance_talk_string(str_no);
    if (text == NULL) {
        mChoice_Load_ChoseStringFromRom(choice, str, str_no, actor);
        return;
    }
    GcMsg m = gc_msg_text(gc_msg_start(str, mChoice_CHOICE_STRING_LEN), text);
    if (!m.ok) {
        fprintf(stderr, "[ordinance] choice string %d doesn't fit\n", str_no);
    }
    memset(str + m.len, CHAR_SPACE, mChoice_CHOICE_STRING_LEN - m.len);
}
