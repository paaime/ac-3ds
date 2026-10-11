#include "quick_save.h"

#define DIRECTIONS     8
#define DIRECTION_STEP 0x2000u /* 45 degrees */

bool quick_save_allowed(QuickSaveCheck c)
{
    bool where = c.in_town_outdoors && !c.on_island && !c.first_intro && !c.title_demo;
    bool when = !c.fading && !c.reset_pending && !c.menu_open && c.player_free;
    bool who = c.house_owner && !c.needs_a_friend;
    return where && when && who;
}

int quick_save_direction(uint16_t angle)
{
    return (int)(((uint32_t)angle + DIRECTION_STEP / 2) / DIRECTION_STEP) % DIRECTIONS;
}
