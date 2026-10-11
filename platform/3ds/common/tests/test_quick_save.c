/* Host tests for quick_save.c. */
#include <stdio.h>

#include "../quick_save.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

/* Outdoors in town, free to open the pockets, the house's owner. */
static QuickSaveCheck free_in_town(void)
{
    QuickSaveCheck c = { true, false, false, false, false, false, false, true, true, false };
    return c;
}

static void test_free_in_town_can_save(void)
{
    CHECK(quick_save_allowed(free_in_town()));
}

static void test_not_where_the_gyroid_couldnt_be_used(void)
{
    QuickSaveCheck c = free_in_town();
    c.in_town_outdoors = false; /* indoors, another scene */
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.on_island = true;
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.first_intro = true;
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.title_demo = true;
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.fading = true;
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.reset_pending = true; /* Mr. Resetti is due */
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.menu_open = true;
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.player_free = false; /* talking, busy */
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.house_owner = false; /* a visitor from another town */
    CHECK(!quick_save_allowed(c));
    c = free_in_town();
    c.needs_a_friend = true; /* the gyroid's first-job rule */
    CHECK(!quick_save_allowed(c));
}

/* The game's door directions: south first, one per 45 degrees (m_scene.c's angle table). */
static void test_facing_to_door_direction(void)
{
    CHECK(quick_save_direction(0x0000) == 0);      /* south */
    CHECK(quick_save_direction(0x0FFF) == 0);
    CHECK(quick_save_direction(0x1000) == 1);      /* rounds to the nearest */
    CHECK(quick_save_direction(0x4000) == 2);      /* east */
    CHECK(quick_save_direction(0x8000) == 4);      /* north */
    CHECK(quick_save_direction(0xE000) == 7);
    CHECK(quick_save_direction(0xF000) == 0);      /* wraps to south */
    CHECK(quick_save_direction(0xFFFF) == 0);
}

int main(void)
{
    test_free_in_town_can_save();
    test_not_where_the_gyroid_couldnt_be_used();
    test_facing_to_door_direction();
    if (failures != 0) {
        fprintf(stderr, "quick_save: %d check(s) failed\n", failures);
        return 1;
    }
    printf("quick_save: all tests passed\n");
    return 0;
}
