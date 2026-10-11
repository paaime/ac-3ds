#ifndef AC3DS_BOTTOM_3DS_H
#define AC3DS_BOTTOM_3DS_H

/* The bottom screen: the town map or the Options page,
 * with Save and Options buttons in the top-right corner. */

#include <stdbool.h>

typedef enum {
    BOTTOM_DEBUG,   /* boot steps and live stats (status_3ds.c) */
    BOTTOM_MAP,
    BOTTOM_OPTIONS,
} BottomView;

bool bottom3ds_init(void);
void bottom3ds_shutdown(void);

/* Between frames, once per input scan (pad_3ds.c), with no citro3d frame open:
 * the first call leaves the boot screen for the map. */
void bottom3ds_input(void);
/* SELECT + R: the debug stats and back. */
void bottom3ds_toggle(void);
/* A new touch at (x, y), screen pixels. */
void bottom3ds_touch(int x, int y);
/* Once per input scan, after any new touch: whether the screen is touched. */
void bottom3ds_touch_held(bool held);

/* Inside the citro3d frame, after the game's draws; true if redrawn. */
bool bottom3ds_draw(void);

#endif /* AC3DS_BOTTOM_3DS_H */
