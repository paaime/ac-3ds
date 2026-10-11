/* Host tests for acre_sync.c. */
#include <stdio.h>

#include "../acre_sync.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static AcreScenes none(void)
{
    return (AcreScenes){ false, false, false };
}

static void test_the_setting_reaches_the_game(void)
{
    AcreSync s = acre_sync(true, false, false, none());
    CHECK(s.write && s.value && s.refresh); /* turned on: the neighbours' actors are set up */
    s = acre_sync(false, true, true, none());
    CHECK(s.write && !s.value && !s.refresh);
}

static void test_nothing_when_they_agree(void)
{
    AcreSync s = acre_sync(true, true, true, none());
    CHECK(!s.write && s.value && !s.refresh);
    s = acre_sync(false, false, false, none());
    CHECK(!s.write && !s.value && !s.refresh);
}

static void test_the_first_days_intro_keeps_the_original(void)
{
    AcreScenes intro = { true, false, false };
    CHECK(!acre_sync(true, false, false, intro).write); /* the game turned it off for the intro */
    AcreSync s = acre_sync(true, true, true, intro);    /* a town saved mid-intro, loaded with it on */
    CHECK(s.write && !s.value && !s.refresh);
}

static void test_the_boat_ride_is_left_alone(void)
{
    AcreScenes ride = { false, true, false };
    CHECK(!acre_sync(true, false, true, ride).write); /* the ride turned it off; it restores it at the end */
    CHECK(!acre_sync(false, true, true, ride).write); /* a change during the ride waits for its end */
    AcreScenes both = { true, true, false };
    CHECK(!acre_sync(true, true, true, both).write);
}

/* Whoever turns it back on (the boat ride's end), the neighbours' actors are
 * set up again: in the original mode they were removed. */
static void test_turned_back_on_by_the_game_refreshes(void)
{
    AcreSync s = acre_sync(true, true, false, none());
    CHECK(!s.write && s.value && s.refresh);
}

/* Not while the player changes acre: the change's last steps would be skipped. */
static void test_an_acre_change_finishes_first(void)
{
    AcreScenes moving = { false, false, true };
    CHECK(!acre_sync(false, true, true, moving).write);
    CHECK(!acre_sync(true, false, false, moving).write);
}

int main(void)
{
    test_the_setting_reaches_the_game();
    test_nothing_when_they_agree();
    test_the_first_days_intro_keeps_the_original();
    test_the_boat_ride_is_left_alone();
    test_turned_back_on_by_the_game_refreshes();
    test_an_acre_change_finishes_first();
    if (failures != 0) {
        fprintf(stderr, "acre_sync: %d check(s) failed\n", failures);
        return 1;
    }
    printf("acre_sync: all tests passed\n");
    return 0;
}
