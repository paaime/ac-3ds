#ifndef AC3DS_QUICK_SAVE_H
#define AC3DS_QUICK_SAVE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool in_town_outdoors;
    bool on_island;
    bool first_intro;
    bool title_demo;
    bool fading;
    bool reset_pending;
    bool menu_open;
    bool player_free; /* mPlib_able_submenu_type1 */
    bool house_owner;
    bool needs_a_friend; /* the gyroid's rule: no save before meeting a villager */
} QuickSaveCheck;

bool quick_save_allowed(QuickSaveCheck c);

/* The mSc_DIRECT_* nearest `angle` (0x10000 a full turn, 0 south). */
int quick_save_direction(uint16_t angle);

#endif /* AC3DS_QUICK_SAVE_H */
