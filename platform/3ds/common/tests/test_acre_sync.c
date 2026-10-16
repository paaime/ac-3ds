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
    CHECK(s.write && s.value && s.refresh);
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
    CHECK(!acre_sync(true, false, false, intro).write);
    AcreSync s = acre_sync(true, true, true, intro);
    CHECK(s.write && !s.value && !s.refresh);
}

static void test_the_boat_ride_is_left_alone(void)
{
    AcreScenes ride = { false, true, false };
    CHECK(!acre_sync(true, false, true, ride).write);
    CHECK(!acre_sync(false, true, true, ride).write);
    AcreScenes both = { true, true, false };
    CHECK(!acre_sync(true, true, true, both).write);
}

static void test_turned_back_on_by_the_game_refreshes(void)
{
    AcreSync s = acre_sync(true, true, false, none());
    CHECK(!s.write && s.value && s.refresh);
}

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
