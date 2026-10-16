#ifndef AC3DS_GX_TEV_CACHE_H
#define AC3DS_GX_TEV_CACHE_H

#include <stdbool.h>
#include <stdint.h>

#include "gx_tev.h"

#define GX_TEV_CACHE_SLOTS 64

typedef bool (*GxTevTranslateFn)(const GxTevConfig* gx, PicaCombiner* out, const char** reason);

typedef struct {
    bool used;
    GxTevConfig key;
    PicaCombiner combiner;
} GxTevCacheSlot;

typedef struct {
    GxTevCacheSlot slots[GX_TEV_CACHE_SLOTS];
    uint32_t hits, misses;
} GxTevCache;

void gx_tev_cache_clear(GxTevCache* cache);

/* NULL if it can't be translated (`reason`, if not NULL, says why); valid until the next call. */
const PicaCombiner* gx_tev_cache_get(GxTevCache* cache, const GxTevConfig* gx, GxTevTranslateFn translate,
                                     const char** reason);

#endif /* AC3DS_GX_TEV_CACHE_H */
