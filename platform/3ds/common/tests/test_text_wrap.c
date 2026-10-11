/* Host tests for text_wrap.c. */
#include <stdio.h>
#include <string.h>

#include "../text_wrap.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static char lines[TEXT_WRAP_MAX_LINES][TEXT_WRAP_MAX_WIDTH + 1];

static void test_short_text_is_one_line(void)
{
    CHECK(text_wrap("No disc image found.", 39, lines, TEXT_WRAP_MAX_LINES) == 1);
    CHECK(strcmp(lines[0], "No disc image found.") == 0);
}

static void test_breaks_between_words(void)
{
    int n = text_wrap("Put your own Animal Crossing disc image in the folder", 20, lines, TEXT_WRAP_MAX_LINES);
    CHECK(n == 3);
    CHECK(strcmp(lines[0], "Put your own Animal") == 0);
    CHECK(strcmp(lines[1], "Crossing disc image") == 0);
    CHECK(strcmp(lines[2], "in the folder") == 0);
    for (int i = 0; i < n; i++) {
        CHECK(strlen(lines[i]) <= 20);
    }
}

static void test_newlines_start_a_new_line_and_blank_lines_stay(void)
{
    int n = text_wrap("First.\n\nSecond.", 39, lines, TEXT_WRAP_MAX_LINES);
    CHECK(n == 3);
    CHECK(strcmp(lines[0], "First.") == 0 && lines[1][0] == '\0' && strcmp(lines[2], "Second.") == 0);
}

static void test_a_word_longer_than_a_line_is_cut(void)
{
    int n = text_wrap("in sdmc:/3ds/ac3ds/averyveryverylongname.iso now", 16, lines, TEXT_WRAP_MAX_LINES);
    CHECK(n >= 3);
    for (int i = 0; i < n; i++) {
        CHECK(strlen(lines[i]) <= 16 && lines[i][0] != ' ');
    }
    CHECK(strcmp(lines[0], "in") == 0);
    CHECK(strncmp(lines[1], "sdmc:/3ds/ac3ds/", 16) == 0);
}

static void test_limits(void)
{
    CHECK(text_wrap("a b c d e f", 1, lines, 3) == 3); /* stops at max_lines */
    CHECK(text_wrap("", 39, lines, TEXT_WRAP_MAX_LINES) == 0);
    CHECK(text_wrap(NULL, 39, lines, TEXT_WRAP_MAX_LINES) == 0);
    CHECK(text_wrap("abc", 0, lines, TEXT_WRAP_MAX_LINES) == 0);
    CHECK(text_wrap("abc", TEXT_WRAP_MAX_WIDTH + 5, lines, TEXT_WRAP_MAX_LINES) == 1); /* clamped */
}

static void test_centring(void)
{
    CHECK(text_center_column(39, 39) == 1);
    CHECK(text_center_column(15, 39) == 13); /* (39 - 15) / 2 + 1 */
    CHECK(text_center_column(50, 39) == 1);
    CHECK(text_center_column(0, 39) == 20);
}

int main(void)
{
    test_short_text_is_one_line();
    test_breaks_between_words();
    test_newlines_start_a_new_line_and_blank_lines_stay();
    test_a_word_longer_than_a_line_is_cut();
    test_limits();
    test_centring();
    if (failures != 0) {
        fprintf(stderr, "text_wrap: %d check(s) failed\n", failures);
        return 1;
    }
    printf("text_wrap: all tests passed\n");
    return 0;
}
