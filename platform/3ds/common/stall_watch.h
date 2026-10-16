#ifndef AC3DS_STALL_WATCH_H
#define AC3DS_STALL_WATCH_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STALL_NONE,
    STALL_BEGAN,
    STALL_GOES_ON,
    STALL_ENDED,
} StallEvent;

typedef struct {
    uint32_t seq;
    uint64_t since_ms;
    uint64_t reported_ms;
    bool reported;
    StallEvent event;
} StallWatch;

StallWatch stall_watch_start(uint32_t seq, uint64_t now_ms);
StallWatch stall_watch_check(StallWatch w, uint32_t seq, uint64_t now_ms, uint64_t limit_ms, uint64_t repeat_ms);

#endif /* AC3DS_STALL_WATCH_H */
