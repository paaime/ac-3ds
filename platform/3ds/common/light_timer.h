#ifndef AC3DS_LIGHT_TIMER_H
#define AC3DS_LIGHT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#define LIGHT_SAVER_TIMEOUT_S 10

typedef struct {
    bool lit;
    uint64_t last_touch_ms;
} LightTimer;

LightTimer light_timer_start(uint64_t now_ms);
LightTimer light_timer_step(LightTimer t, bool saver_on, bool touched, uint64_t now_ms, uint64_t timeout_ms);

#endif /* AC3DS_LIGHT_TIMER_H */
