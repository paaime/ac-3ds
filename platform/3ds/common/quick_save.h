#ifndef AC3DS_QUICK_SAVE_H
#define AC3DS_QUICK_SAVE_H

/* The bottom screen's Save button: the gyroid's save from anywhere in town. */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool in_town_outdoors;
    bool on_island;
    bool first_intro;   /* mEv_CheckFirstIntro */
    bool title_demo;
    bool fading;        /* a scene change or wipe under way */
    bool reset_pending; /* Mr. Resetti is due (reset_flag) */
    bool menu_open;
    bool player_free;   /* mPlib_able_submenu_type1 */
    bool house_owner;   /* not a visitor */
    bool needs_a_friend; /* the gyroid's first-job rule: no save before meeting a villager */
} QuickSaveCheck;

bool quick_save_allowed(QuickSaveCheck c);

/* The door direction (mSc_DIRECT_*: south, then one per 45 degrees) nearest
 * a facing angle (0x10000 a full turn, 0 south). */
int quick_save_direction(uint16_t angle);

#endif /* AC3DS_QUICK_SAVE_H */
