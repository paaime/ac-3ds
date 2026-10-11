#include "light_timer.h"

LightTimer light_timer_start(uint64_t now_ms)
{
    LightTimer t = { true, now_ms };
    return t;
}

LightTimer light_timer_step(LightTimer t, bool saver_on, bool touched, uint64_t now_ms, uint64_t timeout_ms)
{
    if (!saver_on || touched) {
        return light_timer_start(now_ms);
    }
    LightTimer next = t;
    next.lit = now_ms < t.last_touch_ms || now_ms - t.last_touch_ms < timeout_ms;
    return next;
}
