#ifndef AC3DS_BOTTOM_3DS_H
#define AC3DS_BOTTOM_3DS_H

#include <stdbool.h>

typedef enum {
    BOTTOM_DEBUG,
    BOTTOM_MAP,
    BOTTOM_OPTIONS,
} BottomView;

bool bottom3ds_init(void);
void bottom3ds_shutdown(void);

/* Once per input scan, outside the citro3d frame. */
void bottom3ds_input(void);
void bottom3ds_toggle(void);
void bottom3ds_touch(int x, int y);
void bottom3ds_touch_held(bool held);

/* Inside the citro3d frame, after the game's draws; true if redrawn. */
bool bottom3ds_draw(void);

#endif /* AC3DS_BOTTOM_3DS_H */
