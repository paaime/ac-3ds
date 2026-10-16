#ifndef AC3DS_GC_CLOCK_H
#define AC3DS_GC_CLOCK_H

#include <stdint.h>

#define N3DS_SYSTEM_TICK_HZ 268111856u /* svcGetSystemTick rate (libctru SYSCLOCK_ARM11) */

uint64_t gc_clock_convert(uint64_t ticks, uint32_t from_hz, uint32_t to_hz);

/* The system tick stops during sleep: how far to move the game's clock to the RTC's (never back). */
int64_t gc_clock_catch_up_ms(int64_t wall_elapsed_ms, int64_t clock_elapsed_ms, int64_t threshold_ms);

#endif /* AC3DS_GC_CLOCK_H */
