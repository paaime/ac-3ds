#ifndef AC3DS_HEAP_3DS_H
#define AC3DS_HEAP_3DS_H

#include <stdbool.h>
#include <stdint.h>

#include "mem_layout.h"

typedef struct {
    uint32_t available_bytes;
    uint32_t shrink_steps;
    uint32_t base;
    MemLayout layout;
    bool rescue;
    uint32_t needed_bytes;
    uint32_t tight_needed_bytes;
} Heap3dsReport;

Heap3dsReport heap3ds_report(void);

#endif /* AC3DS_HEAP_3DS_H */
