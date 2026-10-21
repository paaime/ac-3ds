#include <stdio.h>
#include <string.h>

#include "../ordinance.h"

static int failures;
#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

#define HOUR_SEC  3600
#define DAY_SEC   (24 * HOUR_SEC)
#define NO_SHIFT  0

static void test_save_byte_round_trip(void)
{
    for (int o = 0; o < ORDINANCE_COUNT; o++) {
        CHECK(ordinance_from_save(ordinance_to_save((Ordinance)o)) == (Ordinance)o);
    }
}

static void test_a_new_town_has_none(void)
{
    CHECK(ordinance_to_save(ORDINANCE_NONE) == 0);
    CHECK(ordinance_from_save(0) == ORDINANCE_NONE);
}

static void test_unknown_save_bytes_read_as_none(void)
{
    CHECK(ordinance_from_save(ORDINANCE_BELL_BOOM) == ORDINANCE_NONE);
    CHECK(ordinance_from_save(0xFF) == ORDINANCE_NONE);
    CHECK(ordinance_from_save((uint8_t)(ordinance_to_save(ORDINANCE_EARLY_BIRD) & 0x0F)) == ORDINANCE_NONE);
    uint8_t past_last = (uint8_t)(ordinance_to_save(ORDINANCE_BEAUTIFUL_TOWN) + 1);
    CHECK(ordinance_from_save(past_last) == ORDINANCE_NONE);
    CHECK(ordinance_to_save((Ordinance)ORDINANCE_COUNT) == 0);
    CHECK(ordinance_to_save((Ordinance)-1) == 0);
}

static void test_villager_time_is_shifted_and_wraps(void)
{
    CHECK(ordinance_villager_sec(ORDINANCE_NONE, 8 * HOUR_SEC) == 8 * HOUR_SEC);
    CHECK(ordinance_villager_sec(ORDINANCE_BELL_BOOM, 8 * HOUR_SEC) == 8 * HOUR_SEC);
    CHECK(ordinance_villager_sec(ORDINANCE_EARLY_BIRD, 6 * HOUR_SEC) == 6 * HOUR_SEC + ORDINANCE_VILLAGER_SHIFT_SEC);
    CHECK(ordinance_villager_sec(ORDINANCE_NIGHT_OWL, 22 * HOUR_SEC) == 22 * HOUR_SEC - ORDINANCE_VILLAGER_SHIFT_SEC);
    CHECK(ordinance_villager_sec(ORDINANCE_EARLY_BIRD, DAY_SEC - 1) == ORDINANCE_VILLAGER_SHIFT_SEC - 1);
    CHECK(ordinance_villager_sec(ORDINANCE_NIGHT_OWL, 0) == DAY_SEC - ORDINANCE_VILLAGER_SHIFT_SEC);
    for (int s = 0; s < DAY_SEC; s += 977) {
        int shifted = ordinance_villager_sec(ORDINANCE_NIGHT_OWL, s);
        CHECK(shifted >= 0 && shifted < DAY_SEC);
    }
}

static void test_villager_time_keeps_odd_input_in_a_day(void)
{
    CHECK(ordinance_villager_sec(ORDINANCE_EARLY_BIRD, DAY_SEC + 10) == (10 + ORDINANCE_VILLAGER_SHIFT_SEC));
    CHECK(ordinance_villager_sec(ORDINANCE_NONE, -5) == -5);
}

static void test_early_bird_opens_shops_early(void)
{
    CHECK(ordinance_shop_open_hour(ORDINANCE_EARLY_BIRD, 9) == ORDINANCE_EARLY_OPEN_HOUR);
    CHECK(ordinance_shop_open_hour(ORDINANCE_EARLY_BIRD, 7) == ORDINANCE_EARLY_OPEN_HOUR);
    CHECK(ordinance_shop_open_hour(ORDINANCE_EARLY_BIRD, 5) == 5);
    CHECK(ordinance_shop_open_hour(ORDINANCE_NONE, 9) == 9);
    CHECK(ordinance_shop_open_hour(ORDINANCE_NIGHT_OWL, 9) == 9);
    CHECK(ordinance_shop_close_hour(ORDINANCE_EARLY_BIRD, 22) == 22);
}

static void test_night_owl_closes_shops_late(void)
{
    CHECK(ordinance_shop_close_hour(ORDINANCE_NIGHT_OWL, 22) == ORDINANCE_LATE_CLOSE_HOUR);
    CHECK(ordinance_shop_close_hour(ORDINANCE_NIGHT_OWL, 23) == ORDINANCE_LATE_CLOSE_HOUR);
    CHECK(ordinance_shop_close_hour(ORDINANCE_NONE, 22) == 22);
    CHECK(ordinance_shop_close_hour(ORDINANCE_BEAUTIFUL_TOWN, 23) == 23);
}

static void test_bell_boom_raises_prices(void)
{
    CHECK(ordinance_item_price(ORDINANCE_NONE, 1000) == 1000);
    CHECK(ordinance_item_price(ORDINANCE_EARLY_BIRD, 1000) == 1000);
    CHECK(ordinance_item_price(ORDINANCE_BELL_BOOM, 1000) == 1000 + 1000 * ORDINANCE_BELL_BOOM_PERCENT / 100);
    CHECK(ordinance_item_price(ORDINANCE_BELL_BOOM, 0) == 0);
    CHECK(ordinance_item_price(ORDINANCE_BELL_BOOM, 3) == 3);
    CHECK(ordinance_item_price(ORDINANCE_BELL_BOOM, 4000000000u) == UINT32_MAX);
}

static void test_beautiful_town_stops_weeds_and_cockroaches(void)
{
    for (int o = 0; o < ORDINANCE_COUNT; o++) {
        bool beautiful = o == ORDINANCE_BEAUTIFUL_TOWN;
        CHECK(ordinance_grows_weeds((Ordinance)o) == !beautiful);
        CHECK(ordinance_brings_cockroaches((Ordinance)o) == !beautiful);
    }
}

static void test_the_fee(void)
{
    CHECK(!ordinance_affordable(0));
    CHECK(!ordinance_affordable(ORDINANCE_FEE - 1));
    CHECK(ordinance_affordable(ORDINANCE_FEE));
    CHECK(ordinance_pay(ORDINANCE_FEE + 5) == 5);
    CHECK(ordinance_pay(ORDINANCE_FEE - 1) == ORDINANCE_FEE - 1);
}

static void test_names(void)
{
    CHECK(strcmp(ordinance_name(ORDINANCE_EARLY_BIRD), "Early Bird") == 0);
    CHECK(strcmp(ordinance_name(ORDINANCE_NIGHT_OWL), "Night Owl") == 0);
    CHECK(strcmp(ordinance_name(ORDINANCE_BELL_BOOM), "Bell Boom") == 0);
    CHECK(strcmp(ordinance_name(ORDINANCE_BEAUTIFUL_TOWN), "Beautiful Town") == 0);
    CHECK(strcmp(ordinance_name(ORDINANCE_NONE), "") == 0);
    CHECK(strcmp(ordinance_name((Ordinance)ORDINANCE_COUNT), "") == 0);
}

int main(void)
{
    test_save_byte_round_trip();
    test_a_new_town_has_none();
    test_unknown_save_bytes_read_as_none();
    test_villager_time_is_shifted_and_wraps();
    test_villager_time_keeps_odd_input_in_a_day();
    test_early_bird_opens_shops_early();
    test_night_owl_closes_shops_late();
    test_bell_boom_raises_prices();
    test_beautiful_town_stops_weeds_and_cockroaches();
    test_the_fee();
    test_names();
    if (failures != 0) {
        fprintf(stderr, "ordinance: %d check(s) failed\n", failures);
        return 1;
    }
    printf("ordinance: all tests passed\n");
    return 0;
}
