#include "gc_clock.h"

uint64_t gc_clock_convert(uint64_t ticks, uint32_t from_hz, uint32_t to_hz)
{
    if (from_hz == 0) {
        return 0;
    }
    /* ticks * to_hz would overflow after ~28 min of 268 MHz ticks: whole seconds first. */
    uint64_t seconds = ticks / from_hz;
    uint64_t rem = ticks % from_hz;
    return seconds * to_hz + rem * to_hz / from_hz;
}

int64_t gc_clock_catch_up_ms(int64_t wall_elapsed_ms, int64_t clock_elapsed_ms, int64_t threshold_ms)
{
    int64_t gap = wall_elapsed_ms - clock_elapsed_ms;
    return gap > threshold_ms ? gap : 0;
}
