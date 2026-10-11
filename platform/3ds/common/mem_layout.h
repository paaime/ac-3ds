#ifndef AC3DS_MEM_LAYOUT_H
#define AC3DS_MEM_LAYOUT_H

/* The port's memory split: all free application memory as one linear block (so
 * every pointer is above 0x30000000, never taken for an N64 segment address),
 * shared by newlib's malloc heap and libctru's linearAlloc pool. When memory is
 * short the pool shrinks first, to its minimum; below both minimums it fails. */

#include <stdbool.h>
#include <stdint.h>

#define MEM_LAYOUT_PAGE 0x1000u

typedef struct {
    bool ok;
    uint32_t total_bytes;  /* page-rounded: malloc_bytes + pool_bytes */
    uint32_t malloc_bytes;
    uint32_t pool_bytes;
    bool tight; /* mem_layout_plan_or_tight: below the heap's usual minimum */
} MemLayout;

/* `available` is rounded down to pages, the other sizes up. */
MemLayout mem_layout_plan(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                          uint32_t min_malloc_bytes);
/* As mem_layout_plan; failing that, the pool at its minimum and the rest for a
 * heap of at least `tight_min_malloc_bytes` (an original 3DS's most memory). */
MemLayout mem_layout_plan_or_tight(uint32_t available, uint32_t pool_bytes, uint32_t min_pool_bytes,
                                   uint32_t min_malloc_bytes, uint32_t tight_min_malloc_bytes);

#endif /* AC3DS_MEM_LAYOUT_H */
