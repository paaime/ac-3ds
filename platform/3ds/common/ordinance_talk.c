#include "ordinance_talk.h"

#include <stdio.h>
#include <string.h>

#define SPEAKERS          2
#define KINDS             (ORDINANCE_COUNT - 1)
#define STRING_YES        3
#define STRING_NO         4
#define STRING_NEVER_MIND 21
#define MARKUP_MAX        1024
#define VALUE_MAX         64
#define BELLS_GROUP       1000u
#define KEY_CHAR          '$'
#define SIGN_COLOUR       "{cffffaf}"
#define SIGN_WIDTH        28
#define NOON              12
#define MIDNIGHT          24

_Static_assert(ORDINANCE_FEE >= BELLS_GROUP && ORDINANCE_FEE < BELLS_GROUP * BELLS_GROUP, "the fee has one comma");

/* The expressions Pelly's and Phyllis's own messages use. */
#define EXPR_SMILE     "{e21}"
#define EXPR_ASK       "{e11}"
#define EXPR_PLAIN     "{e255}"
#define EXPR_SURPRISED "{e4}"
#define EXPR_SORRY     "{e23}"
#define EXPR_ANNOYED   "{e22}"
#define EXPR_BORED     "{e15}"

typedef enum {
    SLOT_INTRO,
    SLOT_ABOUT,
    SLOT_FILED = SLOT_ABOUT + KINDS,
    SLOT_SHORT = SLOT_FILED + KINDS,
    SLOT_ALREADY,
    SLOT_REPEAL,
    SLOT_REPEALED,
    SLOT_DECLINED,
    SLOT_COUNT
} Slot;

_Static_assert(SLOT_COUNT * SPEAKERS == ORDINANCE_TALK_MSG_COUNT, "one message per slot and speaker");

typedef struct {
    const char* name;
    const char* status;
    char fee[VALUE_MAX];
} Values;

typedef struct {
    char* buf;
    size_t size;
    size_t len;
    bool ok;
} Text;

static const char* const k_intro[SPEAKERS] = {
    EXPR_SMILE "Ordinances?{w10} Oh, yes!{p}" EXPR_PLAIN "Mayor Tortimer lets our\nresidents choose one town\n"
               "ordinance, and I take care\nof the paperwork.{p}$status{p}" EXPR_ASK "Which one would you like?",
    EXPR_BORED "Ordinances?{w8} Ugh.{w10}\n{(}More paperwork...{)}{p}" EXPR_ANNOYED "Tortimer lets you folks pick\n"
               "one town ordinance, and I'm\nthe one stuck filing it.{p}$status{p}So?{w6} Which one?",
};

static const char* const k_status_none[SPEAKERS] = {
    "Right now, our town doesn't\nhave an ordinance.",
    "Right now there's no\nordinance. Big surprise.",
};

static const char* const k_status_some[SPEAKERS] = {
    "Right now, the\n$name Ordinance\nis in effect.",
    "Right now we've got the\n$name Ordinance.\n{(}Thrilling.{)}",
};

static const char* const k_about[KINDS][SPEAKERS] = {
    {
        EXPR_SMILE "The Early Bird Ordinance!{p}" EXPR_PLAIN "Everyone in town rises with\nthe sun, and the shops open\n"
                   "early to match.{p}",
        EXPR_BORED "Early Bird, huh?{p}" EXPR_ANNOYED "Everybody's up at the crack\nof dawn, and the shops open\n"
                   "early.\n{(}Who even wants that?{)}{p}",
    },
    {
        EXPR_SMILE "The Night Owl Ordinance!{p}" EXPR_PLAIN "Everyone in town stays up\nuntil the wee hours, and the\n"
                   "shops stay open late, too.{p}",
        EXPR_BORED "Night Owl, huh?{p}" EXPR_ANNOYED "Everybody's up half the\nnight, and the shops stay\n"
                   "open late.\n{(}Lucky them.{)}{p}",
    },
    {
        EXPR_SMILE "The Bell Boom Ordinance!{p}" EXPR_PLAIN "Tom Nook pays more for\nanything you sell...{w10} but\n"
                   "his prices go up, too.{p}",
        EXPR_BORED "Bell Boom, huh?{p}" EXPR_ANNOYED "Nook pays more for your\njunk, but everything in his\n"
                   "shop costs more, too.\n{(}Same old Nook...{)}{p}",
    },
    {
        EXPR_SMILE "Ah, the Beautiful Town\nOrdinance!{p}" EXPR_PLAIN "No new weeds will sprout,\n"
                   "and no cockroaches will\nsneak into anyone's home.{p}",
        EXPR_BORED "Beautiful Town, huh?{p}" EXPR_ANNOYED "No new weeds, and no\ncockroaches in anybody's\n"
                   "house.\n{(}Must be nice.{)}{p}",
    },
};

static const char* const k_confirm[SPEAKERS] = {
    EXPR_ASK "The filing fee is $fee\nBells. Shall I file it?",
    EXPR_PLAIN "Filing costs $fee Bells.\nNo discounts. Yes or no?",
};

static const char* const k_filed[SPEAKERS] = {
    EXPR_SMILE "Wonderful!{w8} That's $fee\nBells, then...{p}" EXPR_PLAIN "All done! The $name\n"
               "Ordinance is now in effect.\nI hope everyone enjoys it!",
    EXPR_BORED "Fine.{w8} $fee Bells.{w8} Hand\n'em over.{p}" EXPR_PLAIN "There. The $name\n"
               "Ordinance is in effect.\n{(}Finally, somebody paid.{)}",
};

static const char* const k_short[SPEAKERS] = {
    EXPR_SORRY "Oh, dear...{w8} You don't have\n$fee Bells with you.{p}" EXPR_PLAIN "Please come back when you\ndo!",
    EXPR_ANNOYED "What? You don't even have\n$fee Bells on you?{p}{(}Unbelievable. Wasting my\n time...{)}",
};

static const char* const k_already[SPEAKERS] = {
    EXPR_SURPRISED "Oh!{w8} That ordinance is\nalready in effect, so\nthere's nothing to file!",
    EXPR_ANNOYED "That one's already in\neffect.{w8} Pay attention!",
};

static const char* const k_repeal[SPEAKERS] = {
    EXPR_PLAIN "You'd like to repeal the\n$name Ordinance?{p}" EXPR_ASK "That's free of charge.\n"
               "Shall I repeal it?",
    EXPR_BORED "Repeal it?{w8} Fine by me.{p}" EXPR_PLAIN "It's free, at least.\nSo? Yes or no?",
};

static const char* const k_repealed[SPEAKERS] = {
    EXPR_SMILE "All done! Our town no\nlonger has an ordinance.",
    EXPR_BORED "There. No more ordinance.\n{(}Less paperwork for me!{)}",
};

static const char* const k_declined[SPEAKERS] = {
    EXPR_PLAIN "Oh, all right. Let me know\nif you change your mind!",
    EXPR_ANNOYED "Then why'd you bother me?!\n{(}Unbelievable...{)}",
};

static const char* const k_strings[ORDINANCE_TALK_STRING_END - ORDINANCE_TALK_STRING_ORDINANCES] = {
    "Ordinances", "Early Bird", "Night Owl", "Bell Boom", "Beautiful Town", "Repeal it",
};

bool ordinance_talk_owns(int msg)
{
    return msg >= ORDINANCE_TALK_MSG_FIRST && msg < ORDINANCE_TALK_MSG_FIRST + ORDINANCE_TALK_MSG_COUNT;
}

static Slot slot_of(int msg)
{
    return (Slot)((msg - ORDINANCE_TALK_MSG_FIRST) / SPEAKERS);
}

static OrdinanceTalkSpeaker speaker_of(int msg)
{
    return (OrdinanceTalkSpeaker)((msg - ORDINANCE_TALK_MSG_FIRST) % SPEAKERS);
}

static int msg_of(Slot slot, OrdinanceTalkSpeaker speaker)
{
    return ORDINANCE_TALK_MSG_FIRST + (int)slot * SPEAKERS + (int)speaker;
}

static bool enactable(Ordinance o)
{
    return o > ORDINANCE_NONE && o < ORDINANCE_COUNT;
}

int ordinance_talk_intro(OrdinanceTalkSpeaker speaker)
{
    return msg_of(SLOT_INTRO, speaker);
}

const char* ordinance_talk_string(int id)
{
    bool ours = id >= ORDINANCE_TALK_STRING_ORDINANCES && id < ORDINANCE_TALK_STRING_END;
    return ours ? k_strings[id - ORDINANCE_TALK_STRING_ORDINANCES] : NULL;
}

static Values values(const char* name, const char* status)
{
    Values v;
    v.name = name;
    v.status = status;
    snprintf(v.fee, sizeof(v.fee), "%u,%03u", ORDINANCE_FEE / BELLS_GROUP, ORDINANCE_FEE % BELLS_GROUP);
    return v;
}

/* An unknown key keeps its '$', which gc_msg refuses: a typo in a template fails the build of its message. */
static const char* value_of(const Values* v, const char* key, size_t* key_len)
{
    static const char* const keys[] = { "name", "status", "fee", "" };
    const char* found[] = { v->name, v->status, v->fee, "$" };
    size_t i = 0;
    while (strncmp(key, keys[i], strlen(keys[i])) != 0) {
        i++;
    }
    *key_len = strlen(keys[i]);
    return found[i];
}

static Text text_put(Text t, const char* s, size_t n)
{
    Text out = t;
    if (!t.ok || t.size - t.len <= n) {
        out.ok = false;
        return out;
    }
    memcpy(out.buf + out.len, s, n);
    out.len += n;
    out.buf[out.len] = '\0';
    return out;
}

static Text text_add(Text t, const char* template, const Values* v)
{
    Text out = t;
    for (const char* p = template; *p != '\0' && out.ok;) {
        if (*p != KEY_CHAR) {
            out = text_put(out, p, 1);
            p++;
            continue;
        }
        size_t key_len = 0;
        const char* value = value_of(v, p + 1, &key_len);
        out = text_put(out, value, strlen(value));
        p += 1 + key_len;
    }
    return out;
}

static Text text_start(char* out, size_t size)
{
    Text t = { out, size, 0, out != NULL && size > 0 };
    if (t.ok) {
        out[0] = '\0';
    }
    return t;
}

static Text intro_text(Text t, OrdinanceTalkSpeaker sp, Ordinance current)
{
    char status[MARKUP_MAX];
    bool some = enactable(current);
    Values named = values(ordinance_name(current), "");
    Text s = text_add(text_start(status, sizeof(status)), some ? k_status_some[sp] : k_status_none[sp], &named);
    Values v = values("", status);
    return s.ok ? text_add(t, k_intro[sp], &v) : s;
}

static Text slot_text(Text t, Slot slot, OrdinanceTalkSpeaker sp, OrdinanceTalkState st)
{
    Values current = values(ordinance_name(st.current), "");
    if (slot == SLOT_INTRO) {
        return intro_text(t, sp, st.current);
    }
    if (slot >= SLOT_ABOUT && slot < SLOT_FILED) {
        return text_add(text_add(t, k_about[slot - SLOT_ABOUT][sp], &current), k_confirm[sp], &current);
    }
    if (slot >= SLOT_FILED && slot < SLOT_SHORT) {
        Values filed = values(ordinance_name((Ordinance)(slot - SLOT_FILED + 1)), "");
        return text_add(t, k_filed[sp], &filed);
    }
    static const char* const* const k_rest[] = { k_short, k_already, k_repeal, k_repealed, k_declined };
    return text_add(t, k_rest[slot - SLOT_SHORT][sp], &current);
}

bool ordinance_talk_markup(int msg, OrdinanceTalkState st, char* out, size_t size)
{
    if (!ordinance_talk_owns(msg)) {
        return false;
    }
    return slot_text(text_start(out, size), slot_of(msg), speaker_of(msg), st).ok;
}

static GcMsgChoice choice(int string_id, Slot next, OrdinanceTalkSpeaker sp)
{
    GcMsgChoice c = { (uint16_t)string_id, (uint16_t)msg_of(next, sp) };
    return c;
}

static int intro_choices(OrdinanceTalkSpeaker sp, Ordinance current, GcMsgChoice out[GC_MSG_CHOICES_MAX])
{
    int n = 0;
    for (int o = ORDINANCE_EARLY_BIRD; o < ORDINANCE_COUNT; o++) {
        Slot next = o == (int)current ? SLOT_ALREADY : (Slot)(SLOT_ABOUT + o - 1);
        out[n++] = choice(ORDINANCE_TALK_STRING_EARLY_BIRD + o - 1, next, sp);
    }
    if (enactable(current)) {
        out[n++] = choice(ORDINANCE_TALK_STRING_REPEAL, SLOT_REPEAL, sp);
    }
    out[n++] = choice(STRING_NEVER_MIND, SLOT_DECLINED, sp);
    return n;
}

int ordinance_talk_choices(int msg, OrdinanceTalkState st, GcMsgChoice out[GC_MSG_CHOICES_MAX])
{
    if (!ordinance_talk_owns(msg)) {
        return 0;
    }
    Slot slot = slot_of(msg);
    OrdinanceTalkSpeaker sp = speaker_of(msg);
    if (slot == SLOT_INTRO) {
        return intro_choices(sp, st.current, out);
    }
    if (slot >= SLOT_ABOUT && slot < SLOT_FILED) {
        Slot filed = (Slot)(SLOT_FILED + (slot - SLOT_ABOUT));
        out[0] = choice(STRING_YES, ordinance_affordable(st.wallet) ? filed : SLOT_SHORT, sp);
        out[1] = choice(STRING_NO, SLOT_DECLINED, sp);
        return 2;
    }
    if (slot == SLOT_REPEAL) {
        out[0] = choice(STRING_YES, SLOT_REPEALED, sp);
        out[1] = choice(STRING_NO, SLOT_DECLINED, sp);
        return 2;
    }
    return 0;
}

size_t ordinance_talk_build(int msg, OrdinanceTalkState st, uint8_t* out, size_t size)
{
    char markup[MARKUP_MAX];
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    if (!ordinance_talk_markup(msg, st, markup, sizeof(markup))) {
        return 0;
    }
    int n = ordinance_talk_choices(msg, st, choices);
    GcMsg m = gc_msg_text(gc_msg_start(out, size), markup);
    m = n > 0 ? gc_msg_menu(m, choices, n) : gc_msg_continue(m);
    return m.ok ? m.len : 0;
}

OrdinanceTalkStep ordinance_talk_step(int msg)
{
    OrdinanceTalkStep step = { ORDINANCE_TALK_NOTHING, ORDINANCE_NONE, false };
    if (!ordinance_talk_owns(msg)) {
        return step;
    }
    Slot slot = slot_of(msg);
    if (slot >= SLOT_FILED && slot < SLOT_SHORT) {
        step.effect = ORDINANCE_TALK_ENACT;
        step.ordinance = (Ordinance)(slot - SLOT_FILED + 1);
    } else if (slot == SLOT_REPEALED) {
        step.effect = ORDINANCE_TALK_REPEAL;
    }
    step.back_to_menu = slot >= SLOT_FILED && slot != SLOT_REPEAL;
    return step;
}

static void hour_text(int hour, char* out, size_t size)
{
    if (hour == MIDNIGHT) {
        snprintf(out, size, "midnight");
    } else if (hour == NOON) {
        snprintf(out, size, "noon");
    } else {
        snprintf(out, size, "%d:00 %s", hour > NOON ? hour - NOON : hour, hour > NOON ? "p.m." : "a.m.");
    }
}

static Text sign_line(Text t, const char* line)
{
    char spaces[SIGN_WIDTH];
    size_t len = strlen(line);
    size_t pad = len < SIGN_WIDTH ? (SIGN_WIDTH - len) / 2 : 0;
    memset(spaces, ' ', sizeof(spaces));
    return text_put(text_put(text_put(t, SIGN_COLOUR, strlen(SIGN_COLOUR)), spaces, pad), line, len);
}

size_t ordinance_talk_sign(Ordinance o, int open_hour, int close_hour, uint8_t* out, size_t size)
{
    if (o != ORDINANCE_EARLY_BIRD && o != ORDINANCE_NIGHT_OWL) {
        return 0;
    }
    char open[VALUE_MAX];
    char close[VALUE_MAX];
    char hours[MARKUP_MAX];
    char name[MARKUP_MAX];
    char markup[MARKUP_MAX];
    hour_text(open_hour, open, sizeof(open));
    hour_text(close_hour, close, sizeof(close));
    snprintf(hours, sizeof(hours), "Open %s - %s", open, close);
    snprintf(name, sizeof(name), "(%s Ordinance)", ordinance_name(o));
    Text t = sign_line(text_start(markup, sizeof(markup)), "Closed for now");
    t = sign_line(text_put(t, "\n", 1), hours);
    t = sign_line(text_put(t, "\n", 1), name);
    GcMsg m = gc_msg_last(gc_msg_text(gc_msg_start(out, size), markup));
    return t.ok && m.ok ? m.len : 0;
}
