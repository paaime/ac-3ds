#ifndef AC3DS_ORDINANCE_H
#define AC3DS_ORDINANCE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ORDINANCE_NONE,
    ORDINANCE_EARLY_BIRD,
    ORDINANCE_NIGHT_OWL,
    ORDINANCE_BELL_BOOM,
    ORDINANCE_BEAUTIFUL_TOWN,
    ORDINANCE_COUNT
} Ordinance;

#define ORDINANCE_FEE                20000u
#define ORDINANCE_VILLAGER_SHIFT_SEC (2 * 60 * 60)
#define ORDINANCE_EARLY_OPEN_HOUR    6
#define ORDINANCE_LATE_CLOSE_HOUR    24
#define ORDINANCE_BELL_BOOM_PERCENT  20

uint8_t ordinance_to_save(Ordinance o);
Ordinance ordinance_from_save(uint8_t b);

int ordinance_villager_sec(Ordinance o, int now_sec);
int ordinance_shop_open_hour(Ordinance o, int hour);
int ordinance_shop_close_hour(Ordinance o, int hour);
uint32_t ordinance_item_price(Ordinance o, uint32_t price);
bool ordinance_grows_weeds(Ordinance o);
bool ordinance_brings_cockroaches(Ordinance o);

bool ordinance_affordable(uint32_t wallet);
uint32_t ordinance_pay(uint32_t wallet);

const char* ordinance_name(Ordinance o);

#endif /* AC3DS_ORDINANCE_H */
