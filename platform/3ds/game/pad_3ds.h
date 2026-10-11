#ifndef AC3DS_PAD_3DS_H
#define AC3DS_PAD_3DS_H

/* The 3DS's own input outside the game's controller (pad_3ds.c). */

/* Once per input scan, after hidScanInput (ac3ds_main.c): SELECT hotkeys,
 * touches on the bottom screen. */
void pad3ds_hotkeys(void);

#endif /* AC3DS_PAD_3DS_H */
