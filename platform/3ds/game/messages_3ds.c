#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

#include "m_font.h"
#include "m_msg.h"

#include "gc_msg.h"
#include "look_3ds.h"
#include "look_talk.h"
#include "messages_3ds.h"
#include "ordinance_3ds.h"
#include "ordinance_talk.h"
#include "port_msg.h"

#define ADDED_CHOICE 3

_Static_assert(PORT_MSG_FIRST == AC3DS_MSG_FIRST, "the port's messages start where the game's end");
_Static_assert(PORT_MSG_FIRST + PORT_MSG_COUNT == AC3DS_MSG_END, "the shim and port_msg.h agree");
_Static_assert(PORT_STRING_FIRST == mChoice_SELECT_STR_NUM, "strings start where the game's end");
_Static_assert(ORDINANCE_TALK_STRING_LEN == mChoice_CHOICE_STRING_LEN, "a choice string's length");
_Static_assert((int)GC_MSG_FORCE_NEXT == (int)mFont_CONT_CODE_SET_FORCE_NEXT, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_NEXT_MESSAGE_F == (int)mFont_CONT_CODE_SET_NEXT_MESSAGE_F, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SOUND_CUT == (int)mFont_CONT_CODE_SOUND_CUT, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SELECT_NO_B == (int)mFont_CONT_CODE_SELECT_NO_B, "gc_msg's control codes are m_font's");
_Static_assert((int)GC_MSG_SELECT_STRING_6 == (int)mFont_CONT_CODE_SET_SELECT_STRING_6, "gc_msg's control codes are m_font's");
_Static_assert(GC_MSG_NEW_LINE == CHAR_NEW_LINE, "gc_msg's new line is m_font's");
_Static_assert(GC_MSG_TAG == CHAR_MESSAGE_TAG, "gc_msg's message tag is m_font's");

static struct {
    int patched_menu;
    u8 patched[mMsg_MSG_BUF_SIZE];
} s_msgs = { -1, { 0 } };

static int code_size(const uint8_t* code)
{
    return mFont_CodeSize_get((u8*)code);
}

int ac3ds_msg_build(int index, unsigned char* buf, int size)
{
    return ordinance3ds_msg_build(index, buf, size) || look3ds_msg_build(index, buf, size);
}

static GcMsgChoice added_choice(int index)
{
    GcMsgChoice c = ordinance3ds_menu_choice(index);
    return c.next_msg != 0 ? c : look3ds_menu_choice(index);
}

void ac3ds_msg_patch(int index, unsigned char* buf, int len, int size)
{
    s_msgs.patched_menu = -1;
    if (ordinance3ds_sign(index, buf, size) || look3ds_patch_rover(index, buf, len, size)) {
        return;
    }
    GcMsgChoice added = added_choice(index);
    if (added.next_msg == 0) {
        return;
    }
    size_t room = size < (int)sizeof(s_msgs.patched) ? (size_t)size : sizeof(s_msgs.patched);
    size_t n = len > 0 ? gc_msg_insert_choice(buf, (size_t)len, s_msgs.patched, room, added, code_size) : 0;
    s_msgs.patched_menu = n != 0 ? index : -1;
    if (n == 0) {
        fprintf(stderr, "[messages] menu %d has no room for choice %d\n", index, added.string_id);
        return;
    }
    memcpy(buf, s_msgs.patched, n);
}

int ac3ds_menu_choice(int choice)
{
    if (s_msgs.patched_menu < 0 || mMsg_Get_msg_num(mMsg_Get_base_window_p()) != s_msgs.patched_menu) {
        return choice;
    }
    if (choice == ADDED_CHOICE) {
        return AC3DS_MENU_CHOICE_ADDED;
    }
    return choice > ADDED_CHOICE ? choice - 1 : choice;
}

void ac3ds_choice_load_string(mChoice_c* choice, u8* str, int str_no, ACTOR* actor)
{
    const char* text = ordinance_talk_string(str_no);
    text = text != NULL ? text : look_talk_string(str_no);
    if (text == NULL) {
        mChoice_Load_ChoseStringFromRom(choice, str, str_no, actor);
        return;
    }
    GcMsg m = gc_msg_text(gc_msg_start(str, mChoice_CHOICE_STRING_LEN), text);
    if (!m.ok) {
        fprintf(stderr, "[messages] choice string %d doesn't fit\n", str_no);
    }
    memset(str + m.len, CHAR_SPACE, mChoice_CHOICE_STRING_LEN - m.len);
}
