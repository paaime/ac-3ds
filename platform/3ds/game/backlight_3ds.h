#ifndef AC3DS_BACKLIGHT_3DS_H
#define AC3DS_BACKLIGHT_3DS_H

/* The bottom screen's light (backlight_3ds.c): with the light saver setting, off
 * after a while without a touch. Always on in the HOME Menu and after quitting. */

#include <stdbool.h>

/* After gfxInitDefault. If the gsp::Lcd service fails, the light just stays on. */
void backlight3ds_init(void);
/* First thing at shutdown (and at exit): the light back on. Idempotent. */
void backlight3ds_exit(void);

/* Once per input scan. True when the new touch only woke the light: it
 * shouldn't also press what's under it. */
bool backlight3ds_scan(bool new_touch, bool held_touch);

#endif /* AC3DS_BACKLIGHT_3DS_H */
