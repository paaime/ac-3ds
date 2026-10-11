/* Host tests for acre_select.c. */
#include <stdio.h>

#include "../acre_select.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define TIMEOUT 15000u

static bool shows(AcreSelect s, int player_col, int player_row, int col, int row)
{
    int c = -2;
    int r = -2;
    acre_select_shown(s, player_col, player_row, &c, &r);
    return c == col && r == row;
}

static void test_the_players_acre_at_first(void)
{
    CHECK(shows(acre_select_none(), 2, 1, 2, 1));
    CHECK(shows(acre_select_none(), -1, -1, -1, -1)); /* off the map */
    CHECK(!acre_select_step(acre_select_none(), 2, 1, 99999, TIMEOUT).active);
}

static void test_a_tap_shows_that_acre(void)
{
    AcreSelect s = acre_select_tap(acre_select_none(), 4, 5, 2, 1, 1000);
    CHECK(s.active && shows(s, 2, 1, 4, 5));
}

static void test_tapping_your_own_acre_goes_back(void)
{
    AcreSelect s = acre_select_tap(acre_select_none(), 4, 5, 2, 1, 1000);
    s = acre_select_tap(s, 2, 1, 2, 1, 2000);
    CHECK(!s.active && shows(s, 2, 1, 2, 1));
}

static void test_it_lasts_a_while(void)
{
    AcreSelect s = acre_select_tap(acre_select_none(), 4, 5, 2, 1, 1000);
    CHECK(acre_select_step(s, 2, 1, 1000 + TIMEOUT - 1, TIMEOUT).active);
    CHECK(!acre_select_step(s, 2, 1, 1000 + TIMEOUT, TIMEOUT).active);
    AcreSelect again = acre_select_tap(s, 4, 5, 2, 1, 9000); /* a new tap starts the wait again */
    CHECK(acre_select_step(again, 2, 1, 1000 + TIMEOUT, TIMEOUT).active);
}

static void test_walking_into_another_acre_goes_back(void)
{
    AcreSelect s = acre_select_tap(acre_select_none(), 4, 5, 2, 1, 1000);
    CHECK(!acre_select_step(s, 3, 1, 1100, TIMEOUT).active);
    CHECK(!acre_select_step(s, -1, -1, 1100, TIMEOUT).active); /* off the map: indoors with no door, a scene change */
}

static void test_a_clock_going_back_keeps_it(void)
{
    AcreSelect s = acre_select_tap(acre_select_none(), 4, 5, 2, 1, 5000);
    CHECK(acre_select_step(s, 2, 1, 4000, TIMEOUT).active);
}

/* Town acres are the world's blocks 1..5 across and 1..6 down, 640 units
 * each; block 0 and the rest are the border, the beach and the sea. */
static void test_acre_of_a_world_position(void)
{
    int col = -2;
    int row = -2;
    CHECK(acre_of_world(3 * 640.0f + 320.0f, 2 * 640.0f + 320.0f, 640.0f, 5, 6, &col, &row) && col == 2 &&
          row == 1); /* B-3 */
    CHECK(acre_of_world(5 * 640.0f + 639.0f, 6 * 640.0f + 639.0f, 640.0f, 5, 6, &col, &row) && col == 4 &&
          row == 5); /* F-5's far corner */
    CHECK(!acre_of_world(100.0f, 2000.0f, 640.0f, 5, 6, &col, &row) && col == -1 && row == -1); /* border */
    CHECK(!acre_of_world(2000.0f, 300.0f, 640.0f, 5, 6, &col, &row));                         /* the cliff */
    CHECK(!acre_of_world(6 * 640.0f + 10.0f, 2000.0f, 640.0f, 5, 6, &col, &row));             /* past column 5 */
    CHECK(!acre_of_world(2000.0f, 9 * 640.0f, 640.0f, 5, 6, &col, &row));                     /* the beach rows */
    CHECK(!acre_of_world(-5.0f, 700.0f, 640.0f, 5, 6, &col, &row));
}

int main(void)
{
    test_the_players_acre_at_first();
    test_a_tap_shows_that_acre();
    test_tapping_your_own_acre_goes_back();
    test_it_lasts_a_while();
    test_walking_into_another_acre_goes_back();
    test_a_clock_going_back_keeps_it();
    test_acre_of_a_world_position();
    if (failures != 0) {
        fprintf(stderr, "acre_select: %d check(s) failed\n", failures);
        return 1;
    }
    printf("acre_select: all tests passed\n");
    return 0;
}
