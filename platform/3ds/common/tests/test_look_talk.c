#include <stdio.h>
#include <string.h>

#include "../look.h"
#include "../look_talk.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define FIRST       LOOK_TALK_MSG_FIRST
#define END         (LOOK_TALK_MSG_FIRST + LOOK_TALK_MSG_COUNT)
#define MARKUP_SIZE 1024
#define MSG_SIZE    1600
#define LINE_CHARS  28
#define PAGE_LINES  4
#define RETURN_MSG  0x0927
#define STRING_LEN  16
#define GROUPS      3
#define PER_GROUP   3

static LookTalkState state(bool at_limit, int tone)
{
    LookTalkState st = { at_limit, tone, RETURN_MSG };
    return st;
}

static int next_of(int msg, int choice)
{
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    int n = look_talk_choices(msg, choices);
    return choice < n ? choices[choice].next_msg : -1;
}

static bool has(const uint8_t* bytes, size_t len, const uint8_t* want, size_t n)
{
    for (size_t i = 0; i + n <= len; i++) {
        if (memcmp(bytes + i, want, n) == 0) {
            return true;
        }
    }
    return false;
}

static bool markup_has(int msg, LookTalkState st, const char* text)
{
    char markup[MARKUP_SIZE];
    return look_talk_markup(msg, st, markup, sizeof(markup)) && strstr(markup, text) != NULL;
}

static void test_messages_follow_the_ordinances(void)
{
    CHECK(FIRST == PORT_MSG_LOOK_FIRST);
    CHECK(!look_talk_owns(FIRST - 1));
    CHECK(look_talk_owns(FIRST));
    CHECK(look_talk_owns(END - 1));
    CHECK(!look_talk_owns(END));
    CHECK(look_talk_speaker(look_talk_start(LOOK_TALK_GYROID)) == LOOK_TALK_GYROID);
    CHECK(look_talk_speaker(look_talk_start(LOOK_TALK_ROVER)) == LOOK_TALK_ROVER);
    CHECK(look_talk_start(LOOK_TALK_GYROID) != look_talk_start(LOOK_TALK_ROVER));
}

static void test_every_message_builds_and_fits(void)
{
    for (int msg = FIRST; msg < END; msg++) {
        for (int limit = 0; limit < 2; limit++) {
            for (int tone = 0; tone < LOOK_TONES; tone++) {
                char markup[MARKUP_SIZE];
                uint8_t bytes[MSG_SIZE];
                CHECK(look_talk_markup(msg, state(limit, tone), markup, sizeof(markup)));
                CHECK(gc_msg_fits(markup, LINE_CHARS, PAGE_LINES));
                CHECK(look_talk_build(msg, state(limit, tone), bytes, sizeof(bytes)) > 0);
            }
        }
    }
    char markup[MARKUP_SIZE];
    uint8_t bytes[MSG_SIZE];
    CHECK(!look_talk_markup(END, state(false, 0), markup, sizeof(markup)));
    CHECK(!look_talk_markup(FIRST, state(false, 0), markup, 8));
    CHECK(look_talk_build(END, state(false, 0), bytes, sizeof(bytes)) == 0);
    CHECK(look_talk_build(FIRST, state(false, 0), bytes, 8) == 0);
}

static void test_the_gyroid_menu(void)
{
    int menu = look_talk_start(LOOK_TALK_GYROID);
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    CHECK(look_talk_choices(menu, choices) == 3);
    CHECK(choices[0].string_id == LOOK_TALK_STRING_FACE);
    CHECK(choices[1].string_id == LOOK_TALK_STRING_TONE);
    CHECK(choices[2].string_id == LOOK_TALK_STRING_DONE);
    CHECK(look_talk_step(menu).effect == LOOK_TALK_START);
    CHECK(look_talk_step(choices[2].next_msg).effect == LOOK_TALK_FINISH);
    for (int i = 0; i < 3; i++) {
        CHECK(look_talk_speaker(choices[i].next_msg) == LOOK_TALK_GYROID);
    }
}

static void test_the_face_steps(void)
{
    int face = next_of(look_talk_start(LOOK_TALK_GYROID), 0);
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    CHECK(look_talk_step(face).effect == LOOK_TALK_NOTHING);
    CHECK(look_talk_choices(face, choices) == 4);
    LookTalkStep next = look_talk_step(choices[0].next_msg);
    LookTalkStep prev = look_talk_step(choices[1].next_msg);
    CHECK(next.effect == LOOK_TALK_FACE && next.delta == 1);
    CHECK(prev.effect == LOOK_TALK_FACE && prev.delta == -1);
    CHECK(look_talk_step(choices[3].next_msg).effect == LOOK_TALK_FACE_BACK);
    CHECK(look_talk_step(choices[2].next_msg).effect == LOOK_TALK_NOTHING);
    CHECK(look_talk_choices(choices[2].next_msg, choices) == 3);
    CHECK(next_of(next_of(face, 0), 0) == next_of(face, 0));
}

static void test_the_tone_steps_name_the_tone(void)
{
    int tone = next_of(look_talk_start(LOOK_TALK_GYROID), 1);
    int lighter = next_of(tone, 0);
    int darker = next_of(tone, 1);
    CHECK(look_talk_step(lighter).effect == LOOK_TALK_TONE && look_talk_step(lighter).delta == -1);
    CHECK(look_talk_step(darker).effect == LOOK_TALK_TONE && look_talk_step(darker).delta == 1);
    CHECK(look_talk_step(next_of(tone, 3)).effect == LOOK_TALK_TONE_BACK);
    CHECK(markup_has(tone, state(false, 3), look_talk_tone_name(3)));
    CHECK(markup_has(darker, state(false, 7), look_talk_tone_name(7)));
    CHECK(markup_has(lighter, state(true, 0), "lightest"));
    CHECK(markup_has(darker, state(true, LOOK_TONES - 1), "darkest"));
    CHECK(!markup_has(darker, state(true, LOOK_TONES - 1), look_talk_tone_name(LOOK_TONES - 1)));
}

static void test_done_goes_back_to_the_gyroid(void)
{
    int done = next_of(look_talk_start(LOOK_TALK_GYROID), 2);
    uint8_t bytes[MSG_SIZE];
    size_t len = look_talk_build(done, state(false, 0), bytes, sizeof(bytes));
    const uint8_t next[] = { GC_MSG_CONTROL, GC_MSG_NEXT_MESSAGE_F, RETURN_MSG >> 8, RETURN_MSG & 0xFF };
    CHECK(len > 0 && has(bytes, len, next, sizeof(next)));
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    CHECK(look_talk_choices(done, choices) == 0);
    CHECK(markup_has(done, state(false, 0), "{name}"));
}

static void test_rover_offers_every_tone_by_name(void)
{
    int ask = look_talk_start(LOOK_TALK_ROVER);
    GcMsgChoice groups[GC_MSG_CHOICES_MAX];
    CHECK(look_talk_step(ask).effect == LOOK_TALK_NOTHING);
    CHECK(look_talk_choices(ask, groups) == GROUPS);
    CHECK(groups[0].string_id == LOOK_TALK_STRING_LIGHT_SHADES);
    CHECK(groups[1].string_id == LOOK_TALK_STRING_MEDIUM_SHADES);
    CHECK(groups[2].string_id == LOOK_TALK_STRING_DARK_SHADES);
    bool offered[LOOK_TONES] = { false };
    for (int g = 0; g < GROUPS; g++) {
        GcMsgChoice tones[GC_MSG_CHOICES_MAX];
        CHECK(look_talk_choices(groups[g].next_msg, tones) == PER_GROUP + 1);
        CHECK(tones[PER_GROUP].next_msg == ask);
        for (int i = 0; i < PER_GROUP; i++) {
            LookTalkStep set = look_talk_step(tones[i].next_msg);
            CHECK(set.effect == LOOK_TALK_SET_TONE);
            CHECK(set.tone == g * PER_GROUP + i);
            CHECK(strcmp(look_talk_string(tones[i].string_id), look_talk_tone_name(set.tone)) == 0);
            CHECK(markup_has(tones[i].next_msg, state(false, 0), look_talk_tone_name(set.tone)));
            CHECK(look_talk_speaker(tones[i].next_msg) == LOOK_TALK_ROVER);
            offered[set.tone] = true;
        }
    }
    for (int t = 0; t < LOOK_TONES; t++) {
        CHECK(offered[t]);
    }
}

static void test_rovers_choice_resumes_his_talk(void)
{
    int set = next_of(next_of(look_talk_start(LOOK_TALK_ROVER), 1), 0);
    uint8_t bytes[MSG_SIZE];
    size_t len = look_talk_build(set, state(false, 0), bytes, sizeof(bytes));
    const uint8_t next[] = { GC_MSG_CONTROL, GC_MSG_NEXT_MESSAGE_F, PORT_MSG_RESUME >> 8, PORT_MSG_RESUME & 0xFF };
    CHECK(len > 0 && has(bytes, len, next, sizeof(next)));
}

static void test_outside_the_talk(void)
{
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    CHECK(look_talk_step(FIRST - 1).effect == LOOK_TALK_NOTHING);
    CHECK(look_talk_choices(FIRST - 1, choices) == 0);
    CHECK(strcmp(look_talk_tone_name(-1), "") == 0);
    CHECK(strcmp(look_talk_tone_name(LOOK_TONES), "") == 0);
}

static void test_choice_strings(void)
{
    for (int id = LOOK_TALK_STRING_CHANGE; id < LOOK_TALK_STRING_END; id++) {
        const char* s = look_talk_string(id);
        uint8_t bytes[STRING_LEN];
        CHECK(s != NULL && strlen(s) <= STRING_LEN && gc_msg_text(gc_msg_start(bytes, sizeof(bytes)), s).ok);
    }
    CHECK(strcmp(look_talk_string(LOOK_TALK_STRING_CHANGE), "Change my look") == 0);
    CHECK(LOOK_TALK_STRING_CHANGE == PORT_STRING_LOOK_FIRST);
    CHECK(look_talk_string(LOOK_TALK_STRING_CHANGE - 1) == NULL);
    CHECK(look_talk_string(LOOK_TALK_STRING_END) == NULL);
}

int main(void)
{
    test_messages_follow_the_ordinances();
    test_every_message_builds_and_fits();
    test_the_gyroid_menu();
    test_the_face_steps();
    test_the_tone_steps_name_the_tone();
    test_done_goes_back_to_the_gyroid();
    test_rover_offers_every_tone_by_name();
    test_rovers_choice_resumes_his_talk();
    test_outside_the_talk();
    test_choice_strings();
    if (failures != 0) {
        fprintf(stderr, "look_talk: %d check(s) failed\n", failures);
        return 1;
    }
    printf("look_talk: all tests passed\n");
    return 0;
}
