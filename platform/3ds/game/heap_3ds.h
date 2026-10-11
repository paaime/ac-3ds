#ifndef AC3DS_HEAP_3DS_H
#define AC3DS_HEAP_3DS_H

/* What __system_allocateHeaps (heap_3ds.c) did at startup, for the boot log. */

#include <stdbool.h>
#include <stdint.h>

#include "mem_layout.h"

typedef struct {
    uint32_t available_bytes; /* free application memory before allocation */
    uint32_t shrink_steps;    /* failed contiguous allocations before success */
    uint32_t base;
    MemLayout layout;
    bool rescue;              /* too little memory for the game: a small heap so boot can say so */
    uint32_t needed_bytes;    /* malloc + pool minimums */
    uint32_t tight_needed_bytes; /* the same with the tight heap (layout.tight) */
} Heap3dsReport;

Heap3dsReport heap3ds_report(void);

#endif /* AC3DS_HEAP_3DS_H */
