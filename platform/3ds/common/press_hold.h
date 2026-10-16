#ifndef AC3DS_PRESS_HOLD_H
#define AC3DS_PRESS_HOLD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PRESS_NONE,
    PRESS_TAP,
    PRESS_HOLD,
} PressEvent;

typedef struct {
    bool active;
    bool fired;
    uint64_t start_ms;
} PressHold;

typedef struct {
    PressHold state;
    PressEvent event;
} PressStep;

PressHold press_hold_idle(void);
PressHold press_hold_begin(uint64_t now_ms);
/* Once per input scan; `held`: the screen is still touched. */
PressStep press_hold_step(PressHold p, bool held, uint64_t now_ms, uint32_t hold_ms);

#endif /* AC3DS_PRESS_HOLD_H */
