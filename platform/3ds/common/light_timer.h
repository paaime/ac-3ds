#ifndef AC3DS_LIGHT_TIMER_H
#define AC3DS_LIGHT_TIMER_H

/* When the bottom screen's light is on (game/backlight_3ds.c): always, or with
 * the light saver until a while after the last touch. */

#include <stdbool.h>
#include <stdint.h>

#define LIGHT_SAVER_TIMEOUT_S 10 /* also in the Options page's note (settings_file.c) */

typedef struct {
    bool lit;
    uint64_t last_touch_ms; /* or when the wait last started */
} LightTimer;

LightTimer light_timer_start(uint64_t now_ms);
LightTimer light_timer_step(LightTimer t, bool saver_on, bool touched, uint64_t now_ms, uint64_t timeout_ms);

#endif /* AC3DS_LIGHT_TIMER_H */
