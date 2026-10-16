#ifndef AC3DS_OS_3DS_H
#define AC3DS_OS_3DS_H

/* Between frames: the system tick stops in sleep, so this catches the game's clock up with the RTC. */
void os3ds_clock_frame(void);

#endif /* AC3DS_OS_3DS_H */
