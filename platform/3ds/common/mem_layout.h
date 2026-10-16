#ifndef AC3DS_MEM_LAYOUT_H
#define AC3DS_MEM_LAYOUT_H

/* All application memory is linear: every pointer is above 0x30000000, never taken for an N64 segment address. */

#include <stdbool.h>
#include <stdint.h>

#define MEM_LAYOUT_PAGE 0x1000u

typedef struct {
    bool ok;
    uint32_t total_bytes;
    uint32_t malloc_bytes;
    uint32_t pool_bytes;
    bool tight;
} MemLayout;

/* `available` is rounded down to pages, the other sizes up. */
MemLayout mem_layout_plan(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                          uint32_t min_malloc_bytes);
/* Failing mem_layout_plan: the pool at its minimum, a heap of at least `tight_min_malloc_bytes` (original 3DS). */
MemLayout mem_layout_plan_or_tight(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                                   uint32_t min_malloc_bytes, uint32_t tight_min_malloc_bytes);

#endif /* AC3DS_MEM_LAYOUT_H */
