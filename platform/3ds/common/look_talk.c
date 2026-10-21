#include "look_talk.h"

#include <stdio.h>
#include <string.h>

#include "look.h"

#define MARKUP_MAX     512
#define FACE_CHOICES   4
#define MENU_CHOICES   3
#define GROUPS         3
#define PER_GROUP      3
#define STRING_GO_BACK 503

#define ROVER_PLAIN "{e254}"
#define ROVER_HAPPY "{e29}"
#define ROVER_LAUGH "{e24}"

typedef enum {
    SLOT_MENU,
    SLOT_MENU_AGAIN,
    SLOT_FACE,
    SLOT_FACE_NEXT,
    SLOT_FACE_PREV,
    SLOT_FACE_BACK,
    SLOT_TONE,
    SLOT_TONE_LIGHTER,
    SLOT_TONE_DARKER,
    SLOT_TONE_BACK,
    SLOT_DONE,
    SLOT_ROVER_ASK,
    SLOT_ROVER_GROUP,
    SLOT_ROVER_SET = SLOT_ROVER_GROUP + GROUPS,
    SLOT_COUNT = SLOT_ROVER_SET + LOOK_TONES
} Slot;

_Static_assert(SLOT_COUNT == LOOK_TALK_MSG_COUNT, "one message per slot");
_Static_assert(LOOK_TALK_STRING_END - LOOK_TALK_STRING_CHANGE == PORT_STRING_LOOK_COUNT,
               "port_msg.h counts every string");
_Static_assert(LOOK_TALK_STRING_END - LOOK_TALK_STRING_TONE_FIRST == LOOK_TONES, "a name for every tone");
_Static_assert(GROUPS * PER_GROUP == LOOK_TONES, "every tone in a group");

static const char* const k_lines[SLOT_ROVER_ASK] = {
    [SLOT_MENU] = "Appearance service.{w8} Which\nfeature shall I adjust?",
    [SLOT_MENU_AGAIN] = "Adjustment stored.{w8} Shall I\nadjust another feature?",
    [SLOT_FACE] = "Current face displayed.{w8}\nPlease select an adjustment.",
    [SLOT_FACE_NEXT] = "Face changed.\nHow is this one?",
    [SLOT_FACE_PREV] = "Face changed.\nHow is this one?",
    [SLOT_FACE_BACK] = "Original face restored.",
    [SLOT_TONE] = "Skin tone: %s.\nPlease select an adjustment.",
    [SLOT_TONE_LIGHTER] = "Tone changed to %s.\nHow is this one?",
    [SLOT_TONE_DARKER] = "Tone changed to %s.\nHow is this one?",
    [SLOT_TONE_BACK] = "Original skin tone restored.",
    [SLOT_DONE] = "Adjustment complete.{w8} You\nlook splendid, {name}.",
};

static const char* const k_limit_lines[2] = {
    "This is the lightest tone\navailable.",
    "This is the darkest tone\navailable.",
};

static const char* const k_rover_ask = ROVER_PLAIN "Say, before we pull in...{w8}\nthe sun's real strong out\nthere!{p}" ROVER_HAPPY
                                                   "So, how would ya describe\nyour skin tone?";
static const char* const k_rover_group = ROVER_PLAIN "Which one's closest?";
static const char* const k_rover_set = ROVER_LAUGH "%s it is!{w8}\nLookin' good, {name}!";

static const char* const k_strings[LOOK_TALK_STRING_TONE_FIRST - LOOK_TALK_STRING_CHANGE] = {
    "Change my look", "Face",    "Skin tone",    "That's all",    "Next face",   "Previous face", "Keep this one",
    "Put it back",    "Lighter", "Darker",       "Light shades",  "Medium shades", "Dark shades",
};

static const char* const k_tone_names[LOOK_TONES] = {
    "Fair", "Light", "Peach", "Golden", "Tan", "Caramel", "Bronze", "Brown", "Deep brown",
};

bool look_talk_owns(int msg)
{
    return msg >= LOOK_TALK_MSG_FIRST && msg < LOOK_TALK_MSG_FIRST + LOOK_TALK_MSG_COUNT;
}

static Slot slot_of(int msg)
{
    return (Slot)(msg - LOOK_TALK_MSG_FIRST);
}

static int msg_of(Slot slot)
{
    return LOOK_TALK_MSG_FIRST + (int)slot;
}

LookTalkSpeaker look_talk_speaker(int msg)
{
    return slot_of(msg) >= SLOT_ROVER_ASK ? LOOK_TALK_ROVER : LOOK_TALK_GYROID;
}

int look_talk_start(LookTalkSpeaker speaker)
{
    return msg_of(speaker == LOOK_TALK_ROVER ? SLOT_ROVER_ASK : SLOT_MENU);
}

const char* look_talk_tone_name(int tone)
{
    return tone >= 0 && tone < LOOK_TONES ? k_tone_names[tone] : "";
}

const char* look_talk_string(int id)
{
    if (id >= LOOK_TALK_STRING_TONE_FIRST && id < LOOK_TALK_STRING_END) {
        return k_tone_names[id - LOOK_TALK_STRING_TONE_FIRST];
    }
    return id >= LOOK_TALK_STRING_CHANGE && id < LOOK_TALK_STRING_TONE_FIRST ? k_strings[id - LOOK_TALK_STRING_CHANGE]
                                                                              : NULL;
}

static const char* line_of(Slot slot, LookTalkState st)
{
    if (slot >= SLOT_ROVER_SET) {
        return k_rover_set;
    }
    if (slot >= SLOT_ROVER_GROUP) {
        return k_rover_group;
    }
    if (slot == SLOT_ROVER_ASK) {
        return k_rover_ask;
    }
    bool limited = st.at_limit && (slot == SLOT_TONE_LIGHTER || slot == SLOT_TONE_DARKER);
    return limited ? k_limit_lines[slot == SLOT_TONE_DARKER] : k_lines[slot];
}

bool look_talk_markup(int msg, LookTalkState st, char* out, size_t size)
{
    if (!look_talk_owns(msg) || out == NULL) {
        return false;
    }
    Slot slot = slot_of(msg);
    const char* name = slot >= SLOT_ROVER_SET ? k_tone_names[slot - SLOT_ROVER_SET] : look_talk_tone_name(st.tone);
    int len = snprintf(out, size, line_of(slot, st), name);
    return len >= 0 && (size_t)len < size;
}

static GcMsgChoice choice(int string_id, int next_msg)
{
    GcMsgChoice c = { (uint16_t)string_id, (uint16_t)next_msg };
    return c;
}

static int rover_choices(Slot slot, GcMsgChoice out[GC_MSG_CHOICES_MAX])
{
    if (slot == SLOT_ROVER_ASK) {
        for (int g = 0; g < GROUPS; g++) {
            out[g] = choice(LOOK_TALK_STRING_LIGHT_SHADES + g, msg_of((Slot)(SLOT_ROVER_GROUP + g)));
        }
        return GROUPS;
    }
    if (slot >= SLOT_ROVER_GROUP && slot < SLOT_ROVER_SET) {
        int first = (slot - SLOT_ROVER_GROUP) * PER_GROUP;
        for (int i = 0; i < PER_GROUP; i++) {
            out[i] = choice(LOOK_TALK_STRING_TONE_FIRST + first + i, msg_of((Slot)(SLOT_ROVER_SET + first + i)));
        }
        out[PER_GROUP] = choice(STRING_GO_BACK, msg_of(SLOT_ROVER_ASK));
        return PER_GROUP + 1;
    }
    return 0;
}

static int adjust_choices(int first, int second, Slot first_next, Slot back, GcMsgChoice out[GC_MSG_CHOICES_MAX])
{
    out[0] = choice(first, msg_of(first_next));
    out[1] = choice(second, msg_of((Slot)(first_next + 1)));
    out[2] = choice(LOOK_TALK_STRING_KEEP, msg_of(SLOT_MENU_AGAIN));
    out[3] = choice(LOOK_TALK_STRING_PUT_BACK, msg_of(back));
    return FACE_CHOICES;
}

int look_talk_choices(int msg, GcMsgChoice out[GC_MSG_CHOICES_MAX])
{
    if (!look_talk_owns(msg)) {
        return 0;
    }
    Slot slot = slot_of(msg);
    if (slot >= SLOT_ROVER_ASK) {
        return rover_choices(slot, out);
    }
    if (slot == SLOT_MENU || slot == SLOT_MENU_AGAIN) {
        out[0] = choice(LOOK_TALK_STRING_FACE, msg_of(SLOT_FACE));
        out[1] = choice(LOOK_TALK_STRING_TONE, msg_of(SLOT_TONE));
        out[2] = choice(LOOK_TALK_STRING_DONE, msg_of(SLOT_DONE));
        return MENU_CHOICES;
    }
    if (slot >= SLOT_FACE && slot <= SLOT_FACE_BACK) {
        return adjust_choices(LOOK_TALK_STRING_NEXT_FACE, LOOK_TALK_STRING_PREVIOUS_FACE, SLOT_FACE_NEXT,
                              SLOT_FACE_BACK, out);
    }
    if (slot >= SLOT_TONE && slot <= SLOT_TONE_BACK) {
        return adjust_choices(LOOK_TALK_STRING_LIGHTER, LOOK_TALK_STRING_DARKER, SLOT_TONE_LIGHTER, SLOT_TONE_BACK,
                              out);
    }
    return 0;
}

size_t look_talk_build(int msg, LookTalkState st, uint8_t* out, size_t size)
{
    char markup[MARKUP_MAX];
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    if (!look_talk_markup(msg, st, markup, sizeof(markup))) {
        return 0;
    }
    Slot slot = slot_of(msg);
    int n = look_talk_choices(msg, choices);
    GcMsg m = gc_msg_text(gc_msg_start(out, size), markup);
    if (n > 0) {
        m = gc_msg_menu(m, choices, n);
    } else if (slot >= SLOT_ROVER_SET) {
        m = gc_msg_continue_to(m, PORT_MSG_RESUME);
    } else {
        m = slot == SLOT_DONE ? gc_msg_continue_to(m, st.return_msg) : gc_msg_continue(m);
    }
    return m.ok ? m.len : 0;
}

LookTalkStep look_talk_step(int msg)
{
    static const LookTalkStep k_steps[SLOT_ROVER_ASK] = {
        [SLOT_MENU] = { LOOK_TALK_START, 0, 0 },
        [SLOT_FACE_NEXT] = { LOOK_TALK_FACE, 1, 0 },
        [SLOT_FACE_PREV] = { LOOK_TALK_FACE, -1, 0 },
        [SLOT_FACE_BACK] = { LOOK_TALK_FACE_BACK, 0, 0 },
        [SLOT_TONE_LIGHTER] = { LOOK_TALK_TONE, -1, 0 },
        [SLOT_TONE_DARKER] = { LOOK_TALK_TONE, 1, 0 },
        [SLOT_TONE_BACK] = { LOOK_TALK_TONE_BACK, 0, 0 },
        [SLOT_DONE] = { LOOK_TALK_FINISH, 0, 0 },
    };
    LookTalkStep none = { LOOK_TALK_NOTHING, 0, 0 };
    if (!look_talk_owns(msg)) {
        return none;
    }
    Slot slot = slot_of(msg);
    if (slot >= SLOT_ROVER_SET) {
        LookTalkStep set = { LOOK_TALK_SET_TONE, 0, slot - SLOT_ROVER_SET };
        return set;
    }
    return slot >= SLOT_ROVER_ASK ? none : k_steps[slot];
}
