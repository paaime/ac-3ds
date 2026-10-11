/* Host tests for settings_file.c. */
#include "settings_file.h"
#include "light_timer.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                 \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void test_parse_reads_each_key(void)
{
    Settings s = settings_parse(settings_defaults(), "skip_resetti = on\n"
                                                     "fast_text = on\n"
                                                     "continuous_camera = on\n"
                                                     "sharp_textures = on\n"
                                                     "picture_shape = 4:3\n"
                                                     "shop_without_visitor = on\n"
                                                     "no_weeds = on\n"
                                                     "bottom_light_saver = on\n"
                                                     "fast_flush = on\n"
                                                     "one_core_audio = on\n"
                                                     "verbose_log = on\n"
                                                     "dump_textures = on\n");
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(settings_get(s, (SettingId)id));
    }
}

static void test_parse_accepts_numbers_spacing_and_crlf(void)
{
    Settings s = settings_parse(settings_defaults(), "  skip_resetti=1\r\npicture_shape =4:3\r\nfast_flush\t=  1");
    CHECK(settings_get(s, SETTING_SKIP_RESETTI));
    CHECK(settings_value(s, SETTING_PICTURE_SHAPE) == PICTURE_4_3);
    CHECK(settings_get(s, SETTING_FAST_FLUSH)); /* last line, no newline */
}

/* Continuous camera is on until turned off (the PC port's default too),
 * Picture shape is Wide; the rest start off. A file without a setting's line
 * keeps its default. */
static void test_defaults(void)
{
    Settings s = settings_defaults();
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(settings_get(s, (SettingId)id) == (id == SETTING_CONTINUOUS_CAMERA));
    }
    CHECK(settings_value(s, SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
    CHECK(!settings_get(settings_parse(s, "skip_resetti = on\n"), SETTING_SHARP_TEXTURES));
    CHECK(settings_get(settings_parse(s, "skip_resetti = on\n"), SETTING_CONTINUOUS_CAMERA));
    CHECK(!settings_get(settings_parse(s, "continuous_camera = off\n"), SETTING_CONTINUOUS_CAMERA));
    char text[SETTINGS_TEXT_MAX];
    settings_format(s, text, sizeof(text));
    CHECK(strstr(text, "continuous_camera = on\n") != NULL);
}

static void test_parse_can_turn_settings_off(void)
{
    Settings on = settings_parse(settings_defaults(), "skip_resetti = on\nverbose_log = on\n");
    Settings s = settings_parse(on, "skip_resetti = off\nverbose_log = 0\n");
    CHECK(!settings_get(s, SETTING_SKIP_RESETTI));
    CHECK(!settings_get(s, SETTING_VERBOSE_LOG));
}

static void test_parse_ignores_comments_unknown_keys_and_bad_values(void)
{
    Settings base = settings_parse(settings_defaults(), "fast_flush = on\n");
    Settings s = settings_parse(base, "# skip_resetti = on\n"
                                      "unknown = on\n"
                                      "fast_flush = maybe\n"
                                      "picture_shape = on\n" /* not one of wide/4:3/stretch */
                                      "picture_shape = 3\n"  /* past its last choice */
                                      "start_view = stats\n" /* a removed setting, in older files */
                                      "no equals sign\n"
                                      "= on\n"
                                      "\n");
    CHECK(settings_get(s, SETTING_FAST_FLUSH)); /* kept from base */
    CHECK(!settings_get(s, SETTING_SKIP_RESETTI));
    CHECK(settings_value(s, SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
}

static void test_parse_survives_null_and_long_lines(void)
{
    Settings base = settings_parse(settings_defaults(), "verbose_log = on\n");
    CHECK(settings_get(settings_parse(base, NULL), SETTING_VERBOSE_LOG));
    char text[600];
    memset(text, 'x', sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    Settings s = settings_parse(base, text);
    CHECK(settings_get(s, SETTING_VERBOSE_LOG));
}

static void test_format_round_trips(void)
{
    Settings s = settings_toggle(settings_toggle(settings_defaults(), SETTING_SKIP_RESETTI), SETTING_PICTURE_SHAPE);
    char text[SETTINGS_TEXT_MAX];
    size_t n = settings_format(s, text, sizeof(text));
    CHECK(n > 0 && n == strlen(text));
    CHECK(strstr(text, "skip_resetti = on\n") != NULL);
    CHECK(strstr(text, "picture_shape = 4:3\n") != NULL);
    CHECK(strstr(text, "start_view") == NULL); /* removed */
    CHECK(strstr(text, "fast_flush = off\n") != NULL);
    Settings back = settings_parse(settings_defaults(), text);
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(settings_value(back, (SettingId)id) == settings_value(s, (SettingId)id));
    }
}

static void test_format_reports_a_buffer_too_small(void)
{
    char text[16];
    CHECK(settings_format(settings_defaults(), text, sizeof(text)) == 0);
    CHECK(settings_format(settings_defaults(), NULL, 0) == 0);
    char full[SETTINGS_TEXT_MAX];
    size_t n = settings_format(settings_defaults(), full, sizeof(full));
    char cut[SETTINGS_TEXT_MAX];
    CHECK(settings_format(settings_defaults(), cut, n) == 0); /* room for all but the last line's end */
}

static void test_toggle_returns_a_copy(void)
{
    Settings s = settings_defaults();
    Settings t = settings_toggle(s, SETTING_FAST_FLUSH);
    CHECK(!settings_get(s, SETTING_FAST_FLUSH));
    CHECK(settings_get(t, SETTING_FAST_FLUSH));
    CHECK(!settings_get(t, SETTING_SKIP_RESETTI));
    CHECK(!settings_get(settings_toggle(t, SETTING_FAST_FLUSH), SETTING_FAST_FLUSH));
    Settings same = settings_toggle(s, SETTING_COUNT); /* out of range: unchanged */
    CHECK(memcmp(&same, &s, sizeof(s)) == 0);
}

static void test_set_and_out_of_range_get(void)
{
    Settings s = settings_set(settings_defaults(), SETTING_ONE_CORE_AUDIO, true);
    CHECK(settings_get(s, SETTING_ONE_CORE_AUDIO));
    CHECK(!settings_get(s, SETTING_COUNT));
    Settings same = settings_set(s, (SettingId)-1, true);
    CHECK(memcmp(&same, &s, sizeof(s)) == 0);
}

static void test_labels_values_and_when_they_apply(void)
{
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(settings_label((SettingId)id)[0] != '\0');
    }
    Settings s = settings_defaults();
    CHECK(strcmp(settings_value_text(s, SETTING_SKIP_RESETTI), "Off") == 0);
    CHECK(strcmp(settings_value_text(settings_toggle(s, SETTING_SKIP_RESETTI), SETTING_SKIP_RESETTI), "On") == 0);
    CHECK(strcmp(settings_value_text(s, SETTING_PICTURE_SHAPE), "Wide") == 0);
    CHECK(strcmp(settings_value_text(settings_toggle(s, SETTING_PICTURE_SHAPE), SETTING_PICTURE_SHAPE), "4:3") == 0);
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(settings_note((SettingId)id)[0] != '\0');
    }
    CHECK(settings_note(SETTING_COUNT)[0] == '\0');
    CHECK(settings_label(SETTING_COUNT)[0] == '\0');
    CHECK(settings_value_text(s, SETTING_COUNT)[0] == '\0');
}

static void test_switches_and_choices(void)
{
    CHECK(settings_is_switch(SETTING_SKIP_RESETTI));
    CHECK(settings_is_switch(SETTING_FAST_FLUSH));
    CHECK(settings_is_switch(SETTING_FAST_TEXT));
    CHECK(settings_is_switch(SETTING_SHARP_TEXTURES));
    CHECK(settings_is_switch(SETTING_DUMP_TEXTURES));
    CHECK(!settings_is_switch(SETTING_PICTURE_SHAPE)); /* Wide, 4:3 or Stretch: none is "off" */
    CHECK(settings_is_switch(SETTING_CONTINUOUS_CAMERA));
    CHECK(!settings_is_switch(SETTING_COUNT));
}

static void test_pages(void)
{
    int seen[SETTING_COUNT] = { 0 };
    for (int p = 0; p < SETTINGS_PAGE_COUNT; p++) {
        int size = settings_page_size((SettingsPage)p);
        CHECK(size > 0 && size <= SETTINGS_PAGE_MAX);
        CHECK(settings_page_name((SettingsPage)p)[0] != '\0');
        SettingId last = (SettingId)-1;
        for (int n = 0; n < size; n++) {
            SettingId id = settings_on_page((SettingsPage)p, n);
            CHECK(id < SETTING_COUNT);
            CHECK((int)id > (int)last); /* in the settings' own order */
            last = id;
            if (id < SETTING_COUNT) {
                seen[id]++;
            }
        }
        CHECK(settings_on_page((SettingsPage)p, size) == SETTING_COUNT);
        CHECK(settings_on_page((SettingsPage)p, -1) == SETTING_COUNT);
    }
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(seen[id] == 1); /* every setting on exactly one page */
    }
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 0) == SETTING_SKIP_RESETTI);
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 1) == SETTING_PICTURE_SHAPE);  /* on the first sheet */
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 2) == SETTING_CONTINUOUS_CAMERA);
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 3) == SETTING_SHARP_TEXTURES);
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 4) == SETTING_FAST_TEXT);      /* the second sheet's first */
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 5) == SETTING_SHOP_WITHOUT_VISITOR);
    CHECK(settings_on_page(SETTINGS_PAGE_GAME, 6) == SETTING_NO_WEEDS); /* the town's helpers together */
    CHECK(settings_page_size(SETTINGS_PAGE_GAME) == 8);
    CHECK(settings_on_page(SETTINGS_PAGE_TESTING, 3) == SETTING_DUMP_TEXTURES); /* a developer's tool, last */
    CHECK(settings_on_page(SETTINGS_PAGE_TESTING, 0) == SETTING_FAST_FLUSH);
    CHECK(settings_page_size(SETTINGS_PAGE_COUNT) == 0);
    CHECK(settings_page_name(SETTINGS_PAGE_COUNT)[0] == '\0');
}

/* The player build (not the developer's) offers the Game page only: the
 * Testing page's settings count as off there, whatever the file says. */
static void test_pages_per_build(void)
{
    CHECK(settings_page_count(true) == SETTINGS_PAGE_COUNT);
    CHECK(settings_page_count(false) == 1);
    CHECK(settings_page_of(SETTING_SKIP_RESETTI) == SETTINGS_PAGE_GAME);
    CHECK(settings_page_of(SETTING_VERBOSE_LOG) == SETTINGS_PAGE_TESTING);
    CHECK(settings_page_of(SETTING_COUNT) == SETTINGS_PAGE_COUNT);
    for (int id = 0; id < SETTING_COUNT; id++) {
        bool testing = settings_page_of((SettingId)id) == SETTINGS_PAGE_TESTING;
        CHECK(settings_offered((SettingId)id, true));
        CHECK(settings_offered((SettingId)id, false) == !testing);
    }
    CHECK(!settings_offered(SETTING_COUNT, true));
}

/* The game's font has pictures at some ASCII codes (';' a droplet, '*' a
 * tilde, '/' a note): page texts keep to what it draws as itself. */
static bool font_safe(const char* s)
{
    for (; *s != '\0'; s++) {
        char c = *s;
        bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!letter && strchr(" .,:'()-!?", c) == NULL) {
            return false;
        }
    }
    return true;
}

static void test_page_texts_suit_the_game_font(void)
{
    Settings on = settings_defaults();
    for (int id = 0; id < SETTING_COUNT; id++) {
        on = settings_set(on, (SettingId)id, true);
    }
    for (int id = 0; id < SETTING_COUNT; id++) {
        CHECK(font_safe(settings_label((SettingId)id)));
        CHECK(font_safe(settings_note((SettingId)id)));
        CHECK(font_safe(settings_value_text(settings_defaults(), (SettingId)id)));
        CHECK(font_safe(settings_value_text(on, (SettingId)id)));
        for (int v = 0; v < settings_choice_count((SettingId)id); v++) {
            CHECK(font_safe(settings_value_text(settings_set_value(on, (SettingId)id, v), (SettingId)id)));
        }
    }
    for (int p = 0; p < SETTINGS_PAGE_COUNT; p++) {
        CHECK(font_safe(settings_page_name((SettingsPage)p)));
    }
}

static void test_light_note_quotes_the_timeout(void)
{
    char seconds[16];
    snprintf(seconds, sizeof(seconds), " %d s", LIGHT_SAVER_TIMEOUT_S);
    CHECK(strstr(settings_note(SETTING_BOTTOM_LIGHT_SAVER), seconds) != NULL);
}

/* The switch also runs the start-up GPU checks: its
 * note says so, and when it applies. */
static void test_detailed_log_note_names_the_gpu_checks(void)
{
    const char* note = settings_note(SETTING_VERBOSE_LOG);
    CHECK(strstr(note, "GPU checks") != NULL);
    CHECK(strstr(note, "next start") != NULL);
}

/* Picture shape is a choice of three: Wide (the default: more of the world at
 * the sides), 4:3 (the GameCube's, with bars) and Stretch (the 4:3 picture
 * stretched). A tap moves to the next, after the last back to the first. */
static void test_picture_shape_has_three_choices(void)
{
    Settings s = settings_defaults();
    CHECK(settings_choice_count(SETTING_PICTURE_SHAPE) == PICTURE_SHAPE_COUNT);
    CHECK(settings_value(s, SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
    Settings t = settings_toggle(s, SETTING_PICTURE_SHAPE);
    CHECK(settings_value(t, SETTING_PICTURE_SHAPE) == PICTURE_4_3);
    t = settings_toggle(t, SETTING_PICTURE_SHAPE);
    CHECK(settings_value(t, SETTING_PICTURE_SHAPE) == PICTURE_STRETCH);
    CHECK(strcmp(settings_value_text(t, SETTING_PICTURE_SHAPE), "Stretch") == 0);
    CHECK(settings_value(settings_toggle(t, SETTING_PICTURE_SHAPE), SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
    CHECK(settings_value(s, SETTING_PICTURE_SHAPE) == PICTURE_WIDE); /* toggling returned copies */
}

/* Older files said "wide" (then a stretched picture) or "4:3", or 0 / 1:
 * those keep working; the new choice reads as "stretch" or 2. */
static void test_picture_shape_in_the_file(void)
{
    Settings base = settings_defaults();
    CHECK(settings_value(settings_parse(base, "picture_shape = stretch\n"), SETTING_PICTURE_SHAPE) == PICTURE_STRETCH);
    CHECK(settings_value(settings_parse(base, "picture_shape = Stretch\n"), SETTING_PICTURE_SHAPE) == PICTURE_STRETCH);
    CHECK(settings_value(settings_parse(base, "picture_shape = 2\n"), SETTING_PICTURE_SHAPE) == PICTURE_STRETCH);
    CHECK(settings_value(settings_parse(base, "picture_shape = 1\n"), SETTING_PICTURE_SHAPE) == PICTURE_4_3);
    Settings four = settings_parse(base, "picture_shape = 4:3\n");
    CHECK(settings_value(settings_parse(four, "picture_shape = wide\n"), SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
    CHECK(settings_value(settings_parse(four, "picture_shape = 0\n"), SETTING_PICTURE_SHAPE) == PICTURE_WIDE);
    CHECK(settings_value(settings_parse(base, "skip_resetti = 2\n"), SETTING_SKIP_RESETTI) == 0); /* a switch has two */
    char text[SETTINGS_TEXT_MAX];
    settings_format(settings_set_value(base, SETTING_PICTURE_SHAPE, PICTURE_STRETCH), text, sizeof(text));
    CHECK(strstr(text, "picture_shape = stretch\n") != NULL);
}

static void test_values_and_their_range(void)
{
    Settings s = settings_defaults();
    CHECK(settings_choice_count(SETTING_SKIP_RESETTI) == 2);
    CHECK(settings_choice_count(SETTING_COUNT) == 0);
    CHECK(settings_value(s, SETTING_COUNT) == 0);
    CHECK(settings_value(settings_set(s, SETTING_FAST_TEXT, true), SETTING_FAST_TEXT) == 1);
    Settings same = settings_set_value(s, SETTING_PICTURE_SHAPE, PICTURE_SHAPE_COUNT); /* no such choice */
    CHECK(memcmp(&same, &s, sizeof(s)) == 0);
    same = settings_set_value(s, SETTING_PICTURE_SHAPE, -1);
    CHECK(memcmp(&same, &s, sizeof(s)) == 0);
    same = settings_set_value(s, SETTING_COUNT, 1);
    CHECK(memcmp(&same, &s, sizeof(s)) == 0);
    CHECK(settings_get(settings_set_value(s, SETTING_PICTURE_SHAPE, PICTURE_4_3), SETTING_PICTURE_SHAPE));
}

/* "No weeds" keeps the game's own flag up (Wisp's wish): it acts as the town
 * loads, which the note says. A switch, off until turned on. */
static void test_no_weeds(void)
{
    CHECK(settings_is_switch(SETTING_NO_WEEDS));
    CHECK(!settings_get(settings_defaults(), SETTING_NO_WEEDS));
    CHECK(settings_get(settings_parse(settings_defaults(), "no_weeds = on\n"), SETTING_NO_WEEDS));
    CHECK(strstr(settings_note(SETTING_NO_WEEDS), "town") != NULL);
    CHECK(settings_offered(SETTING_NO_WEEDS, false)); /* the player build has it */
}

int main(void)
{
    test_parse_reads_each_key();
    test_parse_accepts_numbers_spacing_and_crlf();
    test_defaults();
    test_pages_per_build();
    test_parse_can_turn_settings_off();
    test_parse_ignores_comments_unknown_keys_and_bad_values();
    test_parse_survives_null_and_long_lines();
    test_format_round_trips();
    test_format_reports_a_buffer_too_small();
    test_toggle_returns_a_copy();
    test_set_and_out_of_range_get();
    test_labels_values_and_when_they_apply();
    test_switches_and_choices();
    test_pages();
    test_page_texts_suit_the_game_font();
    test_light_note_quotes_the_timeout();
    test_detailed_log_note_names_the_gpu_checks();
    test_picture_shape_has_three_choices();
    test_picture_shape_in_the_file();
    test_values_and_their_range();
    test_no_weeds();
    if (failures != 0) {
        fprintf(stderr, "settings_file: %d check(s) failed\n", failures);
        return 1;
    }
    printf("settings_file: all tests passed\n");
    return 0;
}
