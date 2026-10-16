#ifndef AC3DS_CLOCK_3DS_H
#define AC3DS_CLOCK_3DS_H

#include <3ds.h>
#include <stdint.h>

/* The system tick stops while the console sleeps: the port's timers only, not the game's clock. */
static inline uint64_t clock3ds_ms(void)
{
    return svcGetSystemTick() / CPU_TICKS_PER_MSEC;
}

#endif /* AC3DS_CLOCK_3DS_H */
