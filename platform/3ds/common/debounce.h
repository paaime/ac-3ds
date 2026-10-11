#ifndef AC3DS_DEBOUNCE_H
#define AC3DS_DEBOUNCE_H

/* Act once things have been quiet for a while (the settings.ini write). */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool pending;
    uint64_t changed_ms;
} Debounce;

Debounce debounce_idle(void);
Debounce debounce_change(Debounce d, uint64_t now_ms);
bool debounce_due(Debounce d, uint64_t now_ms, uint32_t delay_ms);
Debounce debounce_done(Debounce d);

#endif /* AC3DS_DEBOUNCE_H */
