#ifndef AC3DS_ACRE_SYNC_H
#define AC3DS_ACRE_SYNC_H

#include <stdbool.h>

typedef struct {
    bool first_intro;
    bool boat_ride;
    bool acre_change;
} AcreScenes;

typedef struct {
    bool write;
    bool value;   /* true: continuous (borderless) */
    bool refresh; /* aBC_RequestNearbyRefresh */
} AcreSync;

/* Once a frame. The intro and the boat ride set g_mPlib_wade_disabled themselves. */
AcreSync acre_sync(bool continuous, bool current, bool was_on, AcreScenes scenes);

#endif /* AC3DS_ACRE_SYNC_H */
