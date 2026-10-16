#ifndef AC3DS_BACKLIGHT_3DS_H
#define AC3DS_BACKLIGHT_3DS_H

#include <stdbool.h>

/* After gfxInitDefault. */
void backlight3ds_init(void);
void backlight3ds_exit(void);

/* Once per input scan; true when the touch only woke the light and mustn't press anything. */
bool backlight3ds_scan(bool new_touch, bool held_touch);

#endif /* AC3DS_BACKLIGHT_3DS_H */
