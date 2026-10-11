#ifndef AC3DS_QUICK_SAVE_3DS_H
#define AC3DS_QUICK_SAVE_3DS_H

/* The bottom screen's Save button (quick_save_3ds.c). Plain C types:
 * bottom_3ds.c doesn't include the game's headers. */

#include <stdbool.h>

/* Whether the button works now (common/quick_save.h's rules). */
bool quick_save3ds_available(void);

/* Between frames: off to the save room. False if it isn't available. */
bool quick_save3ds_start(void);

#endif /* AC3DS_QUICK_SAVE_3DS_H */
