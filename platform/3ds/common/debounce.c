#include "debounce.h"

Debounce debounce_idle(void)
{
    Debounce d = { false, 0 };
    return d;
}

Debounce debounce_change(Debounce d, uint64_t now_ms)
{
    Debounce out = d;
    out.pending = true;
    out.changed_ms = now_ms;
    return out;
}

bool debounce_due(Debounce d, uint64_t now_ms, uint32_t delay_ms)
{
    return d.pending && now_ms >= d.changed_ms && now_ms - d.changed_ms >= delay_ms;
}

Debounce debounce_done(Debounce d)
{
    Debounce out = d;
    out.pending = false;
    return out;
}
