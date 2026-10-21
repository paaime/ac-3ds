#include <stdio.h>
#include <string.h>

#include "../ordinance_talk.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define FIRST       ORDINANCE_TALK_MSG_FIRST
#define END         (ORDINANCE_TALK_MSG_FIRST + ORDINANCE_TALK_MSG_COUNT)
#define MARKUP_SIZE 1024
#define MSG_SIZE    1600
#define SIGN_CHARS  28
#define SIGN_LINES  3
#define STRING_YES  3
#define STRING_NO   4
#define STRING_NEVER_MIND 21
#define RICH        99999u
#define POOR        (ORDINANCE_FEE - 1)

static OrdinanceTalkState state(Ordinance current, uint32_t wallet)
{
    OrdinanceTalkState st = { current, wallet };
    return st;
}

static int intro(OrdinanceTalkSpeaker speaker)
{
    return ordinance_talk_intro(speaker);
}

static int next_of(int msg, OrdinanceTalkState st, int choice)
{
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    int n = ordinance_talk_choices(msg, st, choices);
    return choice < n ? choices[choice].next_msg : -1;
}

static int about(OrdinanceTalkSpeaker speaker, Ordinance o)
{
    return next_of(intro(speaker), state(ORDINANCE_NONE, RICH), (int)o - 1);
}

static bool markup_has(int msg, OrdinanceTalkState st, const char* text)
{
    char markup[MARKUP_SIZE];
    return ordinance_talk_markup(msg, st, markup, sizeof(markup)) && strstr(markup, text) != NULL;
}

static void test_messages_follow_the_games(void)
{
    CHECK(FIRST == 0x3F91);
    CHECK(!ordinance_talk_owns(FIRST - 1));
    CHECK(ordinance_talk_owns(FIRST));
    CHECK(ordinance_talk_owns(END - 1));
    CHECK(!ordinance_talk_owns(END));
    CHECK(intro(ORDINANCE_TALK_PELLY) == FIRST);
    CHECK(intro(ORDINANCE_TALK_PHYLLIS) == FIRST + 1);
}

static void test_every_message_builds_and_fits_the_window(void)
{
    for (int msg = FIRST; msg < END; msg++) {
        for (int o = 0; o < ORDINANCE_COUNT; o++) {
            for (int rich = 0; rich < 2; rich++) {
                OrdinanceTalkState st = state((Ordinance)o, rich ? RICH : POOR);
                char markup[MARKUP_SIZE];
                uint8_t bytes[MSG_SIZE];
                CHECK(ordinance_talk_markup(msg, st, markup, sizeof(markup)));
                CHECK(gc_msg_fits(markup, ORDINANCE_TALK_LINE_CHARS, ORDINANCE_TALK_PAGE_LINES));
                CHECK(ordinance_talk_build(msg, st, bytes, sizeof(bytes)) > 0);
            }
        }
    }
}

static void test_other_messages_arent_built(void)
{
    char markup[MARKUP_SIZE];
    uint8_t bytes[MSG_SIZE];
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    OrdinanceTalkState st = state(ORDINANCE_NONE, RICH);
    CHECK(!ordinance_talk_markup(FIRST - 1, st, markup, sizeof(markup)));
    CHECK(!ordinance_talk_markup(END, st, markup, sizeof(markup)));
    CHECK(ordinance_talk_build(END, st, bytes, sizeof(bytes)) == 0);
    CHECK(ordinance_talk_choices(END, st, choices) == 0);
    CHECK(ordinance_talk_build(intro(ORDINANCE_TALK_PELLY), st, bytes, 16) == 0);
    CHECK(!ordinance_talk_markup(intro(ORDINANCE_TALK_PELLY), st, markup, 16));
}

static void test_the_intro_offers_each_ordinance(void)
{
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    int n = ordinance_talk_choices(intro(ORDINANCE_TALK_PELLY), state(ORDINANCE_NONE, RICH), choices);
    CHECK(n == 5);
    CHECK(choices[0].string_id == ORDINANCE_TALK_STRING_EARLY_BIRD);
    CHECK(choices[1].string_id == ORDINANCE_TALK_STRING_NIGHT_OWL);
    CHECK(choices[2].string_id == ORDINANCE_TALK_STRING_BELL_BOOM);
    CHECK(choices[3].string_id == ORDINANCE_TALK_STRING_BEAUTIFUL_TOWN);
    CHECK(choices[4].string_id == STRING_NEVER_MIND);
    for (int i = 0; i < 4; i++) {
        CHECK(ordinance_talk_owns(choices[i].next_msg));
    }
    CHECK(ordinance_talk_step(choices[4].next_msg).back_to_menu);
}

static void test_the_intro_offers_a_repeal_and_refuses_the_same_one(void)
{
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    OrdinanceTalkState st = state(ORDINANCE_NIGHT_OWL, RICH);
    int n = ordinance_talk_choices(intro(ORDINANCE_TALK_PELLY), st, choices);
    CHECK(n == 6);
    CHECK(choices[4].string_id == ORDINANCE_TALK_STRING_REPEAL);
    CHECK(choices[5].string_id == STRING_NEVER_MIND);
    CHECK(choices[0].next_msg == about(ORDINANCE_TALK_PELLY, ORDINANCE_EARLY_BIRD));
    CHECK(choices[1].next_msg != about(ORDINANCE_TALK_PELLY, ORDINANCE_NIGHT_OWL));
    OrdinanceTalkStep already = ordinance_talk_step(choices[1].next_msg);
    CHECK(already.effect == ORDINANCE_TALK_NOTHING && already.back_to_menu);
    CHECK(markup_has(intro(ORDINANCE_TALK_PELLY), st, "Night Owl"));
    CHECK(!markup_has(intro(ORDINANCE_TALK_PELLY), state(ORDINANCE_NONE, RICH), "Night Owl"));
}

static void test_filing_needs_the_fee(void)
{
    for (int o = ORDINANCE_EARLY_BIRD; o < ORDINANCE_COUNT; o++) {
        int msg = about(ORDINANCE_TALK_PELLY, (Ordinance)o);
        GcMsgChoice choices[GC_MSG_CHOICES_MAX];
        CHECK(ordinance_talk_choices(msg, state(ORDINANCE_NONE, RICH), choices) == 2);
        CHECK(choices[0].string_id == STRING_YES && choices[1].string_id == STRING_NO);
        OrdinanceTalkStep filed = ordinance_talk_step(choices[0].next_msg);
        CHECK(filed.effect == ORDINANCE_TALK_ENACT && filed.ordinance == (Ordinance)o && filed.back_to_menu);
        OrdinanceTalkStep declined = ordinance_talk_step(choices[1].next_msg);
        CHECK(declined.effect == ORDINANCE_TALK_NOTHING && declined.back_to_menu);
        OrdinanceTalkStep short_of_bells = ordinance_talk_step(next_of(msg, state(ORDINANCE_NONE, POOR), 0));
        CHECK(short_of_bells.effect == ORDINANCE_TALK_NOTHING && short_of_bells.back_to_menu);
        CHECK(markup_has(msg, state(ORDINANCE_NONE, RICH), "20,000"));
        CHECK(markup_has(choices[0].next_msg, state(ORDINANCE_NONE, RICH), ordinance_name((Ordinance)o)));
    }
}

static void test_repealing(void)
{
    OrdinanceTalkState st = state(ORDINANCE_BELL_BOOM, 0);
    int repeal = next_of(intro(ORDINANCE_TALK_PHYLLIS), st, 4);
    CHECK(!ordinance_talk_step(repeal).back_to_menu);
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    CHECK(ordinance_talk_choices(repeal, st, choices) == 2);
    OrdinanceTalkStep repealed = ordinance_talk_step(choices[0].next_msg);
    CHECK(repealed.effect == ORDINANCE_TALK_REPEAL && repealed.back_to_menu);
    CHECK(ordinance_talk_step(choices[1].next_msg).effect == ORDINANCE_TALK_NOTHING);
}

static void test_each_clerk_keeps_her_own_lines(void)
{
    for (int speaker = 0; speaker < 2; speaker++) {
        for (int o = 0; o < ORDINANCE_COUNT; o++) {
            for (int msg = FIRST + speaker; msg < END; msg += 2) {
                GcMsgChoice choices[GC_MSG_CHOICES_MAX];
                int n = ordinance_talk_choices(msg, state((Ordinance)o, RICH), choices);
                for (int i = 0; i < n; i++) {
                    if (ordinance_talk_owns(choices[i].next_msg)) {
                        CHECK((choices[i].next_msg - FIRST) % 2 == speaker);
                    }
                }
            }
        }
    }
    char pelly[MARKUP_SIZE];
    char phyllis[MARKUP_SIZE];
    OrdinanceTalkState st = state(ORDINANCE_NONE, RICH);
    CHECK(ordinance_talk_markup(intro(ORDINANCE_TALK_PELLY), st, pelly, sizeof(pelly)));
    CHECK(ordinance_talk_markup(intro(ORDINANCE_TALK_PHYLLIS), st, phyllis, sizeof(phyllis)));
    CHECK(strcmp(pelly, phyllis) != 0);
}

static void test_only_the_end_of_a_talk_goes_back(void)
{
    OrdinanceTalkStep outside = ordinance_talk_step(FIRST - 1);
    CHECK(outside.effect == ORDINANCE_TALK_NOTHING && !outside.back_to_menu);
    CHECK(!ordinance_talk_step(intro(ORDINANCE_TALK_PELLY)).back_to_menu);
    CHECK(!ordinance_talk_step(about(ORDINANCE_TALK_PELLY, ORDINANCE_BELL_BOOM)).back_to_menu);
    CHECK(!ordinance_talk_step(END).back_to_menu);
}

static void test_choice_strings(void)
{
    const char* names[] = { "Ordinances", "Early Bird", "Night Owl", "Bell Boom", "Beautiful Town", "Repeal it" };
    for (int id = ORDINANCE_TALK_STRING_ORDINANCES; id < ORDINANCE_TALK_STRING_END; id++) {
        const char* s = ordinance_talk_string(id);
        CHECK(s != NULL && strcmp(s, names[id - ORDINANCE_TALK_STRING_ORDINANCES]) == 0);
        CHECK(s != NULL && strlen(s) <= ORDINANCE_TALK_STRING_LEN);
        uint8_t bytes[ORDINANCE_TALK_STRING_LEN];
        CHECK(s != NULL && gc_msg_text(gc_msg_start(bytes, sizeof(bytes)), s).ok);
    }
    CHECK(ordinance_talk_string(ORDINANCE_TALK_STRING_ORDINANCES - 1) == NULL);
    CHECK(ordinance_talk_string(ORDINANCE_TALK_STRING_END) == NULL);
}

static bool has(const uint8_t* bytes, size_t len, const char* text)
{
    size_t n = strlen(text);
    for (size_t i = 0; i + n <= len; i++) {
        if (memcmp(bytes + i, text, n) == 0) {
            return true;
        }
    }
    return false;
}

static bool sign_fits(const uint8_t* bytes, size_t len)
{
    int chars = 0;
    int lines = 1;
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] == GC_MSG_CONTROL) {
            i += bytes[i + 1] == GC_MSG_COLOR ? 4 : 1;
        } else if (bytes[i] == GC_MSG_NEW_LINE) {
            bool ending = i + 2 < len && bytes[i + 1] == GC_MSG_CONTROL && bytes[i + 2] == GC_MSG_LAST;
            lines += ending ? 0 : 1;
            chars = 0;
        } else if (++chars > SIGN_CHARS) {
            return false;
        }
    }
    return lines <= SIGN_LINES;
}

static void test_the_door_sign_shows_the_ordinances_hours(void)
{
    uint8_t bytes[MSG_SIZE];
    size_t len = ordinance_talk_sign(ORDINANCE_EARLY_BIRD, 6, 22, bytes, sizeof(bytes));
    CHECK(len > 0 && has(bytes, len, "6:00 a.m.") && has(bytes, len, "10:00 p.m.") && has(bytes, len, "Early Bird"));
    CHECK(len > 4 && bytes[0] == GC_MSG_CONTROL && bytes[1] == GC_MSG_COLOR);
    CHECK(len > 2 && bytes[len - 2] == GC_MSG_CONTROL && bytes[len - 1] == GC_MSG_LAST);
    CHECK(sign_fits(bytes, len));
    len = ordinance_talk_sign(ORDINANCE_NIGHT_OWL, 7, 24, bytes, sizeof(bytes));
    CHECK(len > 0 && has(bytes, len, "7:00 a.m.") && has(bytes, len, "midnight") && has(bytes, len, "Night Owl"));
    CHECK(sign_fits(bytes, len));
    len = ordinance_talk_sign(ORDINANCE_EARLY_BIRD, 12, 13, bytes, sizeof(bytes));
    CHECK(len > 0 && has(bytes, len, "noon") && has(bytes, len, "1:00 p.m."));
}

static void test_other_ordinances_leave_the_games_sign(void)
{
    uint8_t bytes[MSG_SIZE];
    CHECK(ordinance_talk_sign(ORDINANCE_NONE, 9, 22, bytes, sizeof(bytes)) == 0);
    CHECK(ordinance_talk_sign(ORDINANCE_BELL_BOOM, 9, 22, bytes, sizeof(bytes)) == 0);
    CHECK(ordinance_talk_sign(ORDINANCE_BEAUTIFUL_TOWN, 9, 22, bytes, sizeof(bytes)) == 0);
    CHECK(ordinance_talk_sign(ORDINANCE_EARLY_BIRD, 6, 22, bytes, 8) == 0);
}

int main(void)
{
    test_messages_follow_the_games();
    test_every_message_builds_and_fits_the_window();
    test_other_messages_arent_built();
    test_the_intro_offers_each_ordinance();
    test_the_intro_offers_a_repeal_and_refuses_the_same_one();
    test_filing_needs_the_fee();
    test_repealing();
    test_each_clerk_keeps_her_own_lines();
    test_only_the_end_of_a_talk_goes_back();
    test_choice_strings();
    test_the_door_sign_shows_the_ordinances_hours();
    test_other_ordinances_leave_the_games_sign();
    if (failures != 0) {
        fprintf(stderr, "ordinance_talk: %d check(s) failed\n", failures);
        return 1;
    }
    printf("ordinance_talk: all tests passed\n");
    return 0;
}
