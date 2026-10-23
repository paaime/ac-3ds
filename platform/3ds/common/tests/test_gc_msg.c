#include <stdio.h>
#include <string.h>

#include "../gc_msg.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define BUF_SIZE 256
#define C        GC_MSG_CONTROL

static int code_size(const uint8_t* p)
{
    if (p[0] == GC_MSG_TAG) {
        return 2;
    }
    if (p[0] != GC_MSG_CONTROL) {
        return 1;
    }
    switch (p[1]) {
    case GC_MSG_CURSOR_SET_TIME:
    case GC_MSG_SOUND_CUT:
        return 3;
    case GC_MSG_COLOR:
    case GC_MSG_DEMO_ORDER_NPC0:
        return 5;
    case GC_MSG_NEXT_MESSAGE_0:
    case GC_MSG_NEXT_MESSAGE_1:
    case GC_MSG_NEXT_MESSAGE_2:
    case GC_MSG_NEXT_MESSAGE_3:
    case GC_MSG_NEXT_MESSAGE_4:
    case GC_MSG_NEXT_MESSAGE_5:
        return 4;
    case GC_MSG_SELECT_STRING_2:
        return 6;
    case GC_MSG_SELECT_STRING_3:
        return 8;
    case GC_MSG_SELECT_STRING_4:
        return 10;
    case GC_MSG_SELECT_STRING_5:
        return 12;
    case GC_MSG_SELECT_STRING_6:
        return 14;
    default:
        return 2;
    }
}

static bool same(GcMsg m, const uint8_t* want, size_t n)
{
    return m.ok && m.len == n && memcmp(m.data, want, n) == 0;
}

static void test_plain_text_and_lines(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_text(gc_msg_start(buf, sizeof(buf)), "Hi, you!\nOK?");
    const uint8_t want[] = { 'H', 'i', ',', ' ', 'y', 'o', 'u', '!', GC_MSG_NEW_LINE, 'O', 'K', '?' };
    CHECK(same(m, want, sizeof(want)));
}

static void test_every_allowed_character(void)
{
    uint8_t buf[BUF_SIZE];
    const char* allowed = " !\"%&'(),-.0123456789:<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    GcMsg m = gc_msg_text(gc_msg_start(buf, sizeof(buf)), allowed);
    CHECK(same(m, (const uint8_t*)allowed, strlen(allowed)));
}

static void test_characters_the_font_draws_differently_are_refused(void)
{
    const char* refused = "#$*+/;[\\]^_`|~\t\x7f\x80}";
    for (const char* c = refused; *c != '\0'; c++) {
        uint8_t buf[BUF_SIZE];
        char text[2] = { *c, '\0' };
        CHECK(!gc_msg_text(gc_msg_start(buf, sizeof(buf)), text).ok);
    }
}

static void test_codes(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_text(gc_msg_start(buf, sizeof(buf)), "{e21}A{w10}{p}B{name}");
    const uint8_t want[] = { C, GC_MSG_DEMO_ORDER_NPC0, 0, 0, 21, 'A', C, GC_MSG_CURSOR_SET_TIME, 10,
                             C, GC_MSG_BUTTON, GC_MSG_NEW_LINE, C, GC_MSG_CLEAR, 'B', C, GC_MSG_PLAYER_NAME };
    CHECK(same(m, want, sizeof(want)));
}

static void test_an_aside_is_grey_and_unvoiced_on_each_line(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_text(gc_msg_start(buf, sizeof(buf)), "x{(}a\nb{)}\ny");
    const uint8_t want[] = { 'x',
                             C, GC_MSG_COLOR, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY,
                             C, GC_MSG_SOUND_CUT, 0, '(', 'a', GC_MSG_NEW_LINE,
                             C, GC_MSG_COLOR, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY, GC_MSG_ASIDE_GREY,
                             'b', ')', C, GC_MSG_SOUND_CUT, 1, GC_MSG_NEW_LINE, 'y' };
    CHECK(same(m, want, sizeof(want)));
}

static void test_continue_to_a_message(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_continue_to(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "a"), 0x0927);
    const uint8_t want[] = { 'a', C, GC_MSG_NEXT_MESSAGE_F, 0x09, 0x27, GC_MSG_NEW_LINE, C, GC_MSG_CONTINUE };
    CHECK(same(m, want, sizeof(want)));
}

static void test_a_colour(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_text(gc_msg_start(buf, sizeof(buf)), "{cffffaf}a{c0A1b2C}");
    const uint8_t want[] = { C, GC_MSG_COLOR, 0xFF, 0xFF, 0xAF, 'a', C, GC_MSG_COLOR, 0x0A, 0x1B, 0x2C };
    CHECK(same(m, want, sizeof(want)));
}

static void test_bad_markup_is_refused(void)
{
    const char* bad[] = { "{", "{p", "{x}", "{w}", "{w256}", "{e300}", "{w1a}", "{)}", "{(}a", "{(}{(}a{)}",
                          "{name", "{e-1}", "{c}", "{cffffa}", "{cffffaff}", "{cfffffg}" };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        uint8_t buf[BUF_SIZE];
        CHECK(!gc_msg_text(gc_msg_start(buf, sizeof(buf)), bad[i]).ok);
    }
}

static void test_text_appends_and_failure_sticks(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_text(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "ab"), "cd");
    CHECK(same(m, (const uint8_t*)"abcd", 4));
    GcMsg bad = gc_msg_text(gc_msg_text(m, "#"), "ef");
    CHECK(!bad.ok);
    CHECK(!gc_msg_continue(bad).ok);
}

static void test_running_out_of_room(void)
{
    uint8_t buf[4];
    CHECK(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "abcd").ok);
    CHECK(!gc_msg_text(gc_msg_start(buf, sizeof(buf)), "abcde").ok);
    CHECK(!gc_msg_text(gc_msg_start(buf, sizeof(buf)), "ab{e1}").ok);
    CHECK(!gc_msg_continue(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "abc")).ok);
    CHECK(!gc_msg_text(gc_msg_start(NULL, 0), "a").ok);
}

static void test_continue_and_last(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsg m = gc_msg_continue(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "a"));
    const uint8_t want[] = { 'a', GC_MSG_NEW_LINE, C, GC_MSG_CONTINUE };
    CHECK(same(m, want, sizeof(want)));
    GcMsg last = gc_msg_last(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "a"));
    const uint8_t want_last[] = { 'a', GC_MSG_NEW_LINE, C, GC_MSG_LAST };
    CHECK(same(last, want_last, sizeof(want_last)));
}

static void test_a_menu_ends_the_message(void)
{
    uint8_t buf[BUF_SIZE];
    GcMsgChoice choices[] = { { 0x0003, 0x3F91 }, { 0x0004, 0x08B3 } };
    GcMsg m = gc_msg_menu(gc_msg_text(gc_msg_start(buf, sizeof(buf)), "Q?"), choices, 2);
    const uint8_t want[] = { 'Q', '?', C, GC_MSG_SELECT_STRING_2, 0x00, 0x03, 0x00, 0x04, C, GC_MSG_SELECT_NO_B,
                             C, GC_MSG_BUTTON, C, GC_MSG_SELECT_WINDOW,
                             C, GC_MSG_NEXT_MESSAGE_0, 0x3F, 0x91, C, GC_MSG_NEXT_MESSAGE_1, 0x08, 0xB3,
                             C, GC_MSG_FORCE_NEXT, GC_MSG_NEW_LINE, C, GC_MSG_CONTINUE };
    CHECK(same(m, want, sizeof(want)));
}

static void test_menus_of_every_size(void)
{
    static const uint8_t select_codes[] = { GC_MSG_SELECT_STRING_2, GC_MSG_SELECT_STRING_3, GC_MSG_SELECT_STRING_4,
                                            GC_MSG_SELECT_STRING_5, GC_MSG_SELECT_STRING_6 };
    static const uint8_t next_codes[] = { GC_MSG_NEXT_MESSAGE_0, GC_MSG_NEXT_MESSAGE_1, GC_MSG_NEXT_MESSAGE_2,
                                          GC_MSG_NEXT_MESSAGE_3, GC_MSG_NEXT_MESSAGE_4, GC_MSG_NEXT_MESSAGE_5 };
    GcMsgChoice choices[GC_MSG_CHOICES_MAX];
    for (int i = 0; i < GC_MSG_CHOICES_MAX; i++) {
        choices[i].string_id = (uint16_t)(100 + i);
        choices[i].next_msg = (uint16_t)(200 + i);
    }
    for (int n = 2; n <= GC_MSG_CHOICES_MAX; n++) {
        uint8_t buf[BUF_SIZE];
        GcMsg m = gc_msg_menu(gc_msg_start(buf, sizeof(buf)), choices, n);
        CHECK(m.ok);
        CHECK(buf[1] == select_codes[n - 2]);
        size_t next_at = 2 + 2 * (size_t)n + 6;
        for (int i = 0; i < n; i++) {
            CHECK(buf[next_at + 4 * (size_t)i + 1] == next_codes[i]);
            CHECK(buf[next_at + 4 * (size_t)i + 3] == 200 + i);
        }
    }
    uint8_t buf[BUF_SIZE];
    CHECK(!gc_msg_menu(gc_msg_start(buf, sizeof(buf)), choices, 1).ok);
    CHECK(!gc_msg_menu(gc_msg_start(buf, sizeof(buf)), choices, GC_MSG_CHOICES_MAX + 1).ok);
    uint8_t small[20];
    CHECK(!gc_msg_menu(gc_msg_start(small, sizeof(small)), choices, 2).ok);
}

static size_t game_menu(uint8_t* out, int n)
{
    static const uint8_t select_codes[] = { 0, 0, GC_MSG_SELECT_STRING_2, GC_MSG_SELECT_STRING_3,
                                            GC_MSG_SELECT_STRING_4, GC_MSG_SELECT_STRING_5 };
    static const uint8_t next_codes[] = { GC_MSG_NEXT_MESSAGE_0, GC_MSG_NEXT_MESSAGE_1, GC_MSG_NEXT_MESSAGE_2,
                                          GC_MSG_NEXT_MESSAGE_3, GC_MSG_NEXT_MESSAGE_4 };
    size_t len = 0;
    const uint8_t intro[] = { C, GC_MSG_DEMO_ORDER_NPC0, 0, 0, 0x7F, 'H', 'i', C, GC_MSG_DEMO_ORDER_NPC0, 9, 0, 1 };
    memcpy(out, intro, sizeof(intro));
    len += sizeof(intro);
    out[len++] = C;
    out[len++] = select_codes[n];
    for (int i = 0; i < n; i++) {
        out[len++] = 0;
        out[len++] = (uint8_t)(0x10 + i);
    }
    const uint8_t window[] = { C, GC_MSG_SELECT_NO_B, C, GC_MSG_BUTTON, C, GC_MSG_SELECT_WINDOW };
    memcpy(out + len, window, sizeof(window));
    len += sizeof(window);
    for (int i = 0; i < n; i++) {
        out[len++] = C;
        out[len++] = next_codes[i];
        out[len++] = 0x08;
        out[len++] = (uint8_t)(0xA0 + i);
    }
    const uint8_t end[] = { GC_MSG_NEW_LINE, C, GC_MSG_FORCE_NEXT, GC_MSG_NEW_LINE, C, GC_MSG_CONTINUE };
    memcpy(out + len, end, sizeof(end));
    return len + sizeof(end);
}

static void test_inserting_a_choice_before_the_last(void)
{
    uint8_t in[BUF_SIZE];
    uint8_t out[BUF_SIZE];
    uint8_t want[BUF_SIZE];
    size_t in_len = game_menu(in, 3);
    GcMsgChoice added = { 0x025F, 0x3F91 };
    size_t len = gc_msg_insert_choice(in, in_len, out, sizeof(out), added, code_size);
    size_t want_len = 0;
    const uint8_t intro[] = { C, GC_MSG_DEMO_ORDER_NPC0, 0, 0, 0x7F, 'H', 'i', C, GC_MSG_DEMO_ORDER_NPC0, 9, 0, 1,
                              C, GC_MSG_SELECT_STRING_4, 0, 0x10, 0, 0x11, 0x02, 0x5F, 0, 0x12,
                              C, GC_MSG_SELECT_NO_B, C, GC_MSG_BUTTON, C, GC_MSG_SELECT_WINDOW,
                              C, GC_MSG_NEXT_MESSAGE_0, 0x08, 0xA0, C, GC_MSG_NEXT_MESSAGE_1, 0x08, 0xA1,
                              C, GC_MSG_NEXT_MESSAGE_2, 0x3F, 0x91, C, GC_MSG_NEXT_MESSAGE_3, 0x08, 0xA2,
                              GC_MSG_NEW_LINE, C, GC_MSG_FORCE_NEXT, GC_MSG_NEW_LINE, C, GC_MSG_CONTINUE };
    memcpy(want, intro, sizeof(intro));
    want_len = sizeof(intro);
    CHECK(len == want_len);
    CHECK(len == in_len + 6);
    CHECK(memcmp(out, want, want_len) == 0);
}

static void test_inserting_into_four_and_five_choices(void)
{
    for (int n = 4; n <= 5; n++) {
        uint8_t in[BUF_SIZE];
        uint8_t out[BUF_SIZE];
        size_t in_len = game_menu(in, n);
        GcMsgChoice added = { 0x0260, 0x3F92 };
        size_t len = gc_msg_insert_choice(in, in_len, out, sizeof(out), added, code_size);
        CHECK(len == in_len + 6);
        CHECK(out[13] == (n == 4 ? GC_MSG_SELECT_STRING_5 : GC_MSG_SELECT_STRING_6));
        size_t ids = 14;
        CHECK(out[ids + 2 * (size_t)(n - 1) + 1] == 0x60);
        CHECK(out[ids + 2 * (size_t)n + 1] == 0x10 + n - 1);
        size_t nexts = ids + 2 * (size_t)(n + 1) + 6;
        CHECK(out[nexts + 4 * (size_t)(n - 1) + 1] == (n == 4 ? GC_MSG_NEXT_MESSAGE_3 : GC_MSG_NEXT_MESSAGE_4));
        CHECK(out[nexts + 4 * (size_t)(n - 1) + 3] == 0x92);
        CHECK(out[nexts + 4 * (size_t)n + 1] == (n == 4 ? GC_MSG_NEXT_MESSAGE_4 : GC_MSG_NEXT_MESSAGE_5));
        CHECK(out[nexts + 4 * (size_t)n + 3] == 0xA0 + n - 1);
    }
}

static void test_what_cant_take_another_choice(void)
{
    GcMsgChoice added = { 1, 2 };
    uint8_t out[BUF_SIZE];
    uint8_t text[] = { 'H', 'i', C, GC_MSG_CONTINUE };
    CHECK(gc_msg_insert_choice(text, sizeof(text), out, sizeof(out), added, code_size) == 0);

    GcMsgChoice six[GC_MSG_CHOICES_MAX] = { { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 } };
    uint8_t full[BUF_SIZE];
    GcMsg m = gc_msg_menu(gc_msg_start(full, sizeof(full)), six, GC_MSG_CHOICES_MAX);
    CHECK(gc_msg_insert_choice(full, m.len, out, sizeof(out), added, code_size) == 0);

    uint8_t in[BUF_SIZE];
    size_t in_len = game_menu(in, 3);
    CHECK(gc_msg_insert_choice(in, in_len, out, in_len + 5, added, code_size) == 0);
    CHECK(gc_msg_insert_choice(in, in_len, out, in_len + 6, added, code_size) == in_len + 6);

    size_t last_next = in_len - 6 - 4;
    in[last_next + 1] = GC_MSG_NEXT_MESSAGE_0;
    CHECK(gc_msg_insert_choice(in, in_len, out, sizeof(out), added, code_size) == 0);

    uint8_t cut[] = { C, GC_MSG_SELECT_STRING_2, 0, 1 };
    CHECK(gc_msg_insert_choice(cut, sizeof(cut), out, sizeof(out), added, code_size) == 0);
    uint8_t lone[] = { 'H', 'i', C };
    CHECK(gc_msg_insert_choice(lone, sizeof(lone), out, sizeof(out), added, code_size) == 0);
    uint8_t lone_tag[] = { 'H', 'i', GC_MSG_TAG };
    CHECK(gc_msg_insert_choice(lone_tag, sizeof(lone_tag), out, sizeof(out), added, code_size) == 0);
}

static void test_a_tag_holding_the_control_byte_is_skipped(void)
{
    uint8_t in[BUF_SIZE];
    uint8_t out[BUF_SIZE];
    in[0] = GC_MSG_TAG;
    in[1] = GC_MSG_CONTROL;
    size_t in_len = 2 + game_menu(in + 2, 3);
    GcMsgChoice added = { 0x025F, 0x3F91 };
    size_t len = gc_msg_insert_choice(in, in_len, out, sizeof(out), added, code_size);
    CHECK(len == in_len + 6);
    CHECK(out[0] == GC_MSG_TAG && out[1] == GC_MSG_CONTROL);
    CHECK(out[2 + 13] == GC_MSG_SELECT_STRING_4);
}

static void test_layout_check(void)
{
    CHECK(gc_msg_fits("{e1}Hello{w8} there,\nfriend!{p}Bye.", 13, 2));
    CHECK(!gc_msg_fits("Hello there, friend!", 13, 2));
    CHECK(!gc_msg_fits("a\nb\nc", 13, 2));
    CHECK(gc_msg_fits("a\nb{p}c\nd", 13, 2));
    CHECK(gc_msg_fits("{(}Tch!{)}", 6, 1));
    CHECK(gc_msg_fits("{cffffaf}Open", 4, 1));
    CHECK(!gc_msg_fits("{(}Tch!!{)}", 6, 1));
    CHECK(gc_msg_fits("{name} is nice", GC_MSG_NAME_CHARS + 8, 1));
    CHECK(!gc_msg_fits("{name} is nice", GC_MSG_NAME_CHARS + 7, 1));
    CHECK(!gc_msg_fits("{bad}", 13, 2));
    CHECK(!gc_msg_fits("Hi {p", 13, 2));
}

int main(void)
{
    test_plain_text_and_lines();
    test_every_allowed_character();
    test_characters_the_font_draws_differently_are_refused();
    test_codes();
    test_an_aside_is_grey_and_unvoiced_on_each_line();
    test_bad_markup_is_refused();
    test_text_appends_and_failure_sticks();
    test_running_out_of_room();
    test_continue_and_last();
    test_continue_to_a_message();
    test_a_colour();
    test_a_tag_holding_the_control_byte_is_skipped();
    test_a_menu_ends_the_message();
    test_menus_of_every_size();
    test_inserting_a_choice_before_the_last();
    test_inserting_into_four_and_five_choices();
    test_what_cant_take_another_choice();
    test_layout_check();
    if (failures != 0) {
        fprintf(stderr, "gc_msg: %d check(s) failed\n", failures);
        return 1;
    }
    printf("gc_msg: all tests passed\n");
    return 0;
}
