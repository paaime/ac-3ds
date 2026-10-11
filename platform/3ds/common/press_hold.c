#include "press_hold.h"

PressHold press_hold_idle(void)
{
    PressHold p = { false, false, 0 };
    return p;
}

PressHold press_hold_begin(uint64_t now_ms)
{
    PressHold p = { true, false, now_ms };
    return p;
}

PressStep press_hold_step(PressHold p, bool held, uint64_t now_ms, uint32_t hold_ms)
{
    PressStep s = { p, PRESS_NONE };
    if (!p.active) {
        return s;
    }
    if (!held) {
        s.event = p.fired ? PRESS_NONE : PRESS_TAP;
        s.state = press_hold_idle();
        return s;
    }
    if (!p.fired && now_ms >= p.start_ms && now_ms - p.start_ms >= hold_ms) {
        s.state.fired = true;
        s.event = PRESS_HOLD;
    }
    return s;
}
