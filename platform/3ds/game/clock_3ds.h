#ifndef AC3DS_CLOCK_3DS_H
#define AC3DS_CLOCK_3DS_H

/* Milliseconds from the system tick, for the port's timers. The tick stops
 * while the console sleeps: the game's own clock is os_3ds.c's. */

#include <3ds.h>
#include <stdint.h>

static inline uint64_t clock3ds_ms(void)
{
    return svcGetSystemTick() / CPU_TICKS_PER_MSEC;
}

#endif /* AC3DS_CLOCK_3DS_H */
