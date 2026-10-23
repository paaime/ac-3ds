#include "ordinance.h"

#define SAVE_TAG      0xA0u
#define SAVE_TAG_MASK 0xF0u
#define SAVE_VALUE    0x0Fu
#define HOUR_SEC      (60 * 60)
#define DAY_SEC       (24 * HOUR_SEC)
#define PERCENT       100u

_Static_assert(ORDINANCE_TAILOR_LATE_CLOSE_HOUR < ORDINANCE_EARLY_OPEN_HOUR, "the tailor's hours wrap past midnight");

static bool valid(Ordinance o)
{
    return (unsigned)o < (unsigned)ORDINANCE_COUNT;
}

uint8_t ordinance_to_save(Ordinance o)
{
    return valid(o) && o != ORDINANCE_NONE ? (uint8_t)(SAVE_TAG | (unsigned)o) : 0;
}

Ordinance ordinance_from_save(uint8_t b)
{
    Ordinance o = (Ordinance)(b & SAVE_VALUE);
    return (b & SAVE_TAG_MASK) == SAVE_TAG && valid(o) ? o : ORDINANCE_NONE;
}

static int shift_sec(Ordinance o)
{
    switch (o) {
    case ORDINANCE_EARLY_BIRD:
        return ORDINANCE_VILLAGER_SHIFT_SEC;
    case ORDINANCE_NIGHT_OWL:
        return -ORDINANCE_VILLAGER_SHIFT_SEC;
    default:
        return 0;
    }
}

static int in_a_day(int sec)
{
    int wrapped = sec % DAY_SEC;
    return wrapped < 0 ? wrapped + DAY_SEC : wrapped;
}

int ordinance_villager_sec(Ordinance o, int now_sec)
{
    int shift = shift_sec(o);
    return shift == 0 ? now_sec : in_a_day(now_sec % DAY_SEC + shift);
}

int ordinance_shop_open_hour(Ordinance o, int hour)
{
    return o == ORDINANCE_EARLY_BIRD && hour > ORDINANCE_EARLY_OPEN_HOUR ? ORDINANCE_EARLY_OPEN_HOUR : hour;
}

int ordinance_shop_close_hour(Ordinance o, int hour)
{
    return o == ORDINANCE_NIGHT_OWL && hour < ORDINANCE_LATE_CLOSE_HOUR ? ORDINANCE_LATE_CLOSE_HOUR : hour;
}

int ordinance_tailor_open_hour(Ordinance o)
{
    return ordinance_shop_open_hour(o, ORDINANCE_TAILOR_OPEN_HOUR);
}

int ordinance_tailor_close_hour(Ordinance o)
{
    return o == ORDINANCE_NIGHT_OWL ? ORDINANCE_TAILOR_LATE_CLOSE_HOUR : ORDINANCE_TAILOR_CLOSE_HOUR;
}

bool ordinance_tailor_open(Ordinance o, int now_sec)
{
    int sec = in_a_day(now_sec);
    return sec < ordinance_tailor_close_hour(o) * HOUR_SEC || sec >= ordinance_tailor_open_hour(o) * HOUR_SEC;
}

uint32_t ordinance_item_price(Ordinance o, uint32_t price)
{
    if (o != ORDINANCE_BELL_BOOM) {
        return price;
    }
    uint64_t raised = (uint64_t)price * (PERCENT + ORDINANCE_BELL_BOOM_PERCENT) / PERCENT;
    return raised > UINT32_MAX ? UINT32_MAX : (uint32_t)raised;
}

bool ordinance_grows_weeds(Ordinance o)
{
    return o != ORDINANCE_BEAUTIFUL_TOWN;
}

bool ordinance_brings_cockroaches(Ordinance o)
{
    return o != ORDINANCE_BEAUTIFUL_TOWN;
}

bool ordinance_affordable(uint32_t wallet)
{
    return wallet >= ORDINANCE_FEE;
}

uint32_t ordinance_pay(uint32_t wallet)
{
    return ordinance_affordable(wallet) ? wallet - ORDINANCE_FEE : wallet;
}

const char* ordinance_name(Ordinance o)
{
    static const char* const k_names[ORDINANCE_COUNT] = {
        [ORDINANCE_NONE] = "",
        [ORDINANCE_EARLY_BIRD] = "Early Bird",
        [ORDINANCE_NIGHT_OWL] = "Night Owl",
        [ORDINANCE_BELL_BOOM] = "Bell Boom",
        [ORDINANCE_BEAUTIFUL_TOWN] = "Beautiful Town",
    };
    return valid(o) ? k_names[o] : "";
}
