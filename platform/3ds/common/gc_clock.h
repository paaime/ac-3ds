#ifndef AC3DS_GC_CLOCK_H
#define AC3DS_GC_CLOCK_H

/* GameCube OS clock on 3DS. OSGetTime counts the GC timer (40.5 MHz); the PC
 * port's elapsed * 40.5e6 / host_hz in 64 bits overflows after about 28 minutes
 * with the 268 MHz system tick, so this splits quotient and remainder. */

#include <stdint.h>

#define N3DS_SYSTEM_TICK_HZ 268111856u /* svcGetSystemTick rate (libctru SYSCLOCK_ARM11) */

/* Rounded down; 0 if from_hz is 0. Exact while both rates fit in 32 bits. */
uint64_t gc_clock_convert(uint64_t ticks, uint32_t from_hz, uint32_t to_hz);

/* The system tick stops during sleep: how far to move the game's clock forward
 * to the RTC's, if over `threshold_ms`. Never backwards: the game's timers
 * expect a clock that only grows. */
int64_t gc_clock_catch_up_ms(int64_t wall_elapsed_ms, int64_t clock_elapsed_ms, int64_t threshold_ms);

#endif /* AC3DS_GC_CLOCK_H */
