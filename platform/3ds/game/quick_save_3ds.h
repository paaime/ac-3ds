#ifndef AC3DS_QUICK_SAVE_3DS_H
#define AC3DS_QUICK_SAVE_3DS_H

#include <stdbool.h>

bool quick_save3ds_available(void);

/* Between frames: off to the save room; false if the button isn't available. */
bool quick_save3ds_start(void);

#endif /* AC3DS_QUICK_SAVE_3DS_H */
