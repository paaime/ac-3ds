#ifndef AC3DS_OS_3DS_H
#define AC3DS_OS_3DS_H

/* The 3DS's Dolphin OS (os_3ds.c). */

/* Once a frame, between frames: after a sleep, when the system tick stops,
 * moves the game's clock forward to the wall clock (common/gc_clock.h). */
void os3ds_clock_frame(void);

#endif /* AC3DS_OS_3DS_H */
