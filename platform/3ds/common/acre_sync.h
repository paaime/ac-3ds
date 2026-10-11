#ifndef AC3DS_ACRE_SYNC_H
#define AC3DS_ACRE_SYNC_H

/* The "Continuous camera" setting against the game's borderless-acre switch
 * (g_mPlib_wade_disabled), which the first day's intro (m_event.c) and the boat
 * ride (ac_boat_demo_move.c_inc) turn off and restore themselves. */

#include <stdbool.h>

typedef struct {
    bool first_intro; /* mEv_CheckFirstIntro */
    bool boat_ride;
    bool acre_change; /* mFI_GetPlayerWade: start or in progress */
} AcreScenes;

typedef struct {
    bool write;
    bool value;   /* true: continuous (borderless) */
    bool refresh; /* set up the neighbouring acres' actors (aBC_RequestNearbyRefresh) */
} AcreSync;

/* Once a frame. The intro keeps the original mode (even from a mid-intro save);
 * a change waits out the boat ride and an acre change. */
AcreSync acre_sync(bool continuous, bool current, bool was_on, AcreScenes scenes);

#endif /* AC3DS_ACRE_SYNC_H */
